#include "Core/DeterministicSeed/DeterministicSeed.h"
#include <random>
#include <functional>
#include <sstream>
#include <iomanip>
#include <vector>

namespace Core {

// ── helpers ──────────────────────────────────────────────────────────────────
// Simple non-cryptographic hash mix (splitmix64 step).
static uint64_t Mix64(uint64_t x) {
    x += 0x9e3779b97f4a7c15ULL;
    x  = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x  = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

// djb2 variant for string hashing.
static uint64_t HashStr(const std::string& s) {
    uint64_t h = 5381;
    for (unsigned char c : s) h = ((h << 5) + h) + c;
    return h;
}

// ── Impl ─────────────────────────────────────────────────────────────────────
struct DeterministicSeed::Impl {
    uint64_t master{0};
    std::vector<DeterministicSeed::SeedChangedCallback> callbacks;
};

void DeterministicSeed::ensureImpl() {
    if (!m_impl) m_impl = new Impl();
}

DeterministicSeed& DeterministicSeed::Get() {
    static DeterministicSeed instance;
    instance.ensureImpl();
    return instance;
}

void DeterministicSeed::SetMasterSeed(uint64_t seed) {
    ensureImpl();
    m_impl->master = seed;
    for (auto& cb : m_impl->callbacks) cb(seed);
}

uint64_t DeterministicSeed::RandomiseMaster() {
    ensureImpl();
    std::random_device rd;
    uint32_t hi = rd();
    uint32_t lo = rd();
    uint64_t s = (static_cast<uint64_t>(hi) << 32) | static_cast<uint64_t>(lo);
    SetMasterSeed(s);
    return s;
}

uint64_t DeterministicSeed::GetMasterSeed() const {
    return m_impl ? m_impl->master : 0;
}

uint64_t DeterministicSeed::Derive(const std::string& domain) const {
    uint64_t base = m_impl ? m_impl->master : 0;
    return Mix64(base ^ HashStr(domain));
}

uint64_t DeterministicSeed::Derive(const std::string& domain,
                                    uint64_t index) const {
    uint64_t base = m_impl ? m_impl->master : 0;
    return Mix64(Mix64(base ^ HashStr(domain)) ^ index);
}

std::string DeterministicSeed::Serialise() const {
    uint64_t v = m_impl ? m_impl->master : 0;
    std::ostringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << v;
    return ss.str();
}

bool DeterministicSeed::Deserialise(const std::string& hex) {
    try {
        uint64_t v = std::stoull(hex, nullptr, 16);
        SetMasterSeed(v);
        return true;
    } catch (...) {
        return false;
    }
}

void DeterministicSeed::OnSeedChanged(SeedChangedCallback cb) {
    ensureImpl();
    m_impl->callbacks.push_back(std::move(cb));
}

} // namespace Core
