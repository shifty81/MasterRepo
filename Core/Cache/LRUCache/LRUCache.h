#pragma once
/**
 * @file LRUCache.h
 * @brief Header-only thread-safe LRU cache with eviction callbacks.
 *
 * LRUCache<K, V, Capacity> keeps the most-recently-used entries up to a fixed
 * capacity. When the cache is full and a new key is inserted, the least-recently-
 * used entry is evicted. An optional eviction callback is fired on every eviction.
 *
 * Usage:
 *   Core::LRUCache<std::string, int, 128> cache;
 *   cache.Put("a", 1);
 *   auto v = cache.Get("a");      // returns std::optional<int>{1}
 *   bool hit = cache.Contains("a");
 *   cache.Evict("a");
 *
 *   // Eviction callback (called when an entry is evicted, including by Put):
 *   cache.SetEvictionCallback([](const std::string& k, const int& v) {
 *       printf("evicted %s -> %d\n", k.c_str(), v);
 *   });
 *
 * Thread safety:
 *   All public methods are protected by a shared std::mutex.
 *
 * Design:
 *   - O(1) Get and Put via a std::unordered_map + intrusive doubly-linked list.
 *   - The list head is the MRU end; the tail is the LRU end.
 *   - No heap allocation per entry — all nodes live in a fixed-size pool array.
 */

#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <utility>

namespace Core {

// ── LRU cache stats ──────────────────────────────────────────────────────
struct LRUStats {
    uint64_t hits{0};
    uint64_t misses{0};
    uint64_t inserts{0};
    uint64_t evictions{0};
    uint64_t manualEvictions{0};

    double HitRate() const {
        uint64_t total = hits + misses;
        return total > 0 ? static_cast<double>(hits) / static_cast<double>(total) : 0.0;
    }
};

// ── LRUCache ─────────────────────────────────────────────────────────────
template<typename K, typename V, size_t Capacity>
class LRUCache {
    static_assert(Capacity > 0, "LRUCache capacity must be > 0");

public:
    using EvictionCallback = std::function<void(const K&, const V&)>;

    LRUCache() { InitFreeList(); }

    // Not copyable; movable with care (mutex is not movable in general use).
    LRUCache(const LRUCache&) = delete;
    LRUCache& operator=(const LRUCache&) = delete;

    // ── eviction callback ─────────────────────────────────────
    void SetEvictionCallback(EvictionCallback cb) {
        std::lock_guard<std::mutex> lk(m_mtx);
        m_onEvict = std::move(cb);
    }

    // ── Put ───────────────────────────────────────────────────
    /// Insert or update key → value.  If the key already exists, move it to MRU.
    /// If the cache is full, evict the LRU entry first.
    void Put(const K& key, const V& value) {
        std::lock_guard<std::mutex> lk(m_mtx);
        auto it = m_map.find(key);
        if (it != m_map.end()) {
            // Update existing — move to MRU
            Node* node   = it->second;
            node->value  = value;
            MoveToFront(node);
            return;
        }
        if (m_size == Capacity) {
            EvictLRU();
        }
        Node* node  = AllocNode();
        node->key   = key;
        node->value = value;
        PushFront(node);
        m_map[key] = node;
        ++m_size;
        ++m_stats.inserts;
    }

    void Put(const K& key, V&& value) {
        std::lock_guard<std::mutex> lk(m_mtx);
        auto it = m_map.find(key);
        if (it != m_map.end()) {
            Node* node   = it->second;
            node->value  = std::move(value);
            MoveToFront(node);
            return;
        }
        if (m_size == Capacity) {
            EvictLRU();
        }
        Node* node  = AllocNode();
        node->key   = key;
        node->value = std::move(value);
        PushFront(node);
        m_map[key] = node;
        ++m_size;
        ++m_stats.inserts;
    }

    // ── Get ───────────────────────────────────────────────────
    /// Returns the value for key, moving it to MRU position, or nullopt on miss.
    std::optional<V> Get(const K& key) {
        std::lock_guard<std::mutex> lk(m_mtx);
        auto it = m_map.find(key);
        if (it == m_map.end()) {
            ++m_stats.misses;
            return std::nullopt;
        }
        ++m_stats.hits;
        MoveToFront(it->second);
        return it->second->value;
    }

    /// Peek at the value without changing recency.
    std::optional<V> Peek(const K& key) const {
        std::lock_guard<std::mutex> lk(m_mtx);
        auto it = m_map.find(key);
        if (it == m_map.end()) return std::nullopt;
        return it->second->value;
    }

    // ── Contains ──────────────────────────────────────────────
    bool Contains(const K& key) const {
        std::lock_guard<std::mutex> lk(m_mtx);
        return m_map.find(key) != m_map.end();
    }

    // ── Evict ─────────────────────────────────────────────────
    /// Manually evict a specific key. Returns true if the key existed.
    bool Evict(const K& key) {
        std::lock_guard<std::mutex> lk(m_mtx);
        auto it = m_map.find(key);
        if (it == m_map.end()) return false;
        Node* node = it->second;
        if (m_onEvict) m_onEvict(node->key, node->value);
        Unlink(node);
        FreeNode(node);
        m_map.erase(it);
        --m_size;
        ++m_stats.manualEvictions;
        return true;
    }

    // ── Clear ─────────────────────────────────────────────────
    void Clear() {
        std::lock_guard<std::mutex> lk(m_mtx);
        if (m_onEvict) {
            // Fire callback for every entry in LRU order
            Node* cur = m_tail;
            while (cur) {
                m_onEvict(cur->key, cur->value);
                cur = cur->prev;
            }
        }
        m_map.clear();
        m_head = m_tail = nullptr;
        m_size = 0;
        InitFreeList();
    }

    // ── queries ───────────────────────────────────────────────
    size_t Size()     const { std::lock_guard<std::mutex> lk(m_mtx); return m_size; }
    bool   IsEmpty()  const { std::lock_guard<std::mutex> lk(m_mtx); return m_size == 0; }
    bool   IsFull()   const { std::lock_guard<std::mutex> lk(m_mtx); return m_size == Capacity; }
    constexpr size_t GetCapacity() const noexcept { return Capacity; }

    LRUStats Stats() const { std::lock_guard<std::mutex> lk(m_mtx); return m_stats; }
    void     ResetStats()  { std::lock_guard<std::mutex> lk(m_mtx); m_stats = {}; }

private:
    // ── intrusive doubly-linked list node ─────────────────────
    struct Node {
        K     key{};
        V     value{};
        Node* prev{nullptr};
        Node* next{nullptr};
        bool  inUse{false};
    };

    // ── list helpers (called under lock) ─────────────────────
    void PushFront(Node* node) {
        node->prev = nullptr;
        node->next = m_head;
        if (m_head) m_head->prev = node;
        m_head = node;
        if (!m_tail) m_tail = node;
    }

    void Unlink(Node* node) {
        if (node->prev) node->prev->next = node->next;
        else            m_head = node->next;
        if (node->next) node->next->prev = node->prev;
        else            m_tail = node->prev;
        node->prev = node->next = nullptr;
    }

    void MoveToFront(Node* node) {
        if (node == m_head) return;
        Unlink(node);
        PushFront(node);
    }

    // ── LRU eviction ─────────────────────────────────────────
    void EvictLRU() {
        if (!m_tail) return;
        Node* lru = m_tail;
        if (m_onEvict) m_onEvict(lru->key, lru->value);
        m_map.erase(lru->key);
        Unlink(lru);
        FreeNode(lru);
        --m_size;
        ++m_stats.evictions;
    }

    // ── fixed pool allocation ─────────────────────────────────
    void InitFreeList() {
        m_freeHead = nullptr;
        for (size_t i = 0; i < Capacity; ++i) {
            m_pool[i].inUse = false;
            m_pool[i].prev  = nullptr;
            m_pool[i].next  = m_freeHead;
            m_freeHead      = &m_pool[i];
        }
    }

    Node* AllocNode() {
        assert(m_freeHead && "LRUCache: AllocNode called on a full pool — EvictLRU must run first");
        Node* n    = m_freeHead;
        m_freeHead = n->next;
        n->inUse   = true;
        n->prev    = n->next = nullptr;
        return n;
    }

    void FreeNode(Node* n) {
        n->inUse = false;
        n->key   = K{};
        n->value = V{};
        n->prev  = nullptr;
        n->next  = m_freeHead;
        m_freeHead = n;
    }

    // ── members ───────────────────────────────────────────────
    mutable std::mutex              m_mtx;
    std::unordered_map<K, Node*>    m_map;
    Node                            m_pool[Capacity]{};
    Node*                           m_freeHead{nullptr};
    Node*                           m_head{nullptr};   // MRU end
    Node*                           m_tail{nullptr};   // LRU end
    size_t                          m_size{0};
    LRUStats                        m_stats{};
    EvictionCallback                m_onEvict;
};

} // namespace Core
