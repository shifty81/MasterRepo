#include "Runtime/Leaderboard/LeaderboardSystem.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace Runtime {

static uint64_t NowMs() {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
}

struct Board {
    LeaderboardInfo          info;
    std::vector<LeaderboardEntry> entries;
};

struct LeaderboardSystem::Impl {
    std::unordered_map<std::string, Board>  boards;
    std::string                             persistPath;

    std::function<void(const std::string&, const LeaderboardEntry&)> onNewTop;
    std::function<void(const std::string&, const std::string&, uint32_t)> onRankChanged;

    void Rank(Board& b) {
        bool desc = b.info.order == ScoreOrder::Descending;
        std::stable_sort(b.entries.begin(), b.entries.end(),
            [desc](const LeaderboardEntry& a, const LeaderboardEntry& b_){
                return desc ? a.score > b_.score : a.score < b_.score;
            });
        for (uint32_t i = 0; i < b.entries.size(); ++i)
            b.entries[i].rank = i + 1;
        // Prune to maxEntries
        if (b.info.maxEntries > 0 && b.entries.size() > b.info.maxEntries)
            b.entries.resize(b.info.maxEntries);
    }
};

LeaderboardSystem::LeaderboardSystem() : m_impl(new Impl()) {}
LeaderboardSystem::~LeaderboardSystem() { delete m_impl; }

void LeaderboardSystem::Init(const std::string& path) {
    m_impl->persistPath = path;
    Load();
}

void LeaderboardSystem::Shutdown() { Save(); }

void LeaderboardSystem::CreateBoard(const std::string& name, ScoreOrder order,
                                    uint32_t maxEntries) {
    Board& b = m_impl->boards[name];
    b.info.name       = name;
    b.info.order      = order;
    b.info.maxEntries = maxEntries;
}

void LeaderboardSystem::DeleteBoard(const std::string& name) {
    m_impl->boards.erase(name);
}

bool LeaderboardSystem::HasBoard(const std::string& name) const {
    return m_impl->boards.count(name) > 0;
}

LeaderboardInfo LeaderboardSystem::GetBoardInfo(const std::string& name) const {
    auto it = m_impl->boards.find(name);
    if (it == m_impl->boards.end()) return {};
    LeaderboardInfo inf = it->second.info;
    inf.entryCount = static_cast<uint32_t>(it->second.entries.size());
    return inf;
}

std::vector<LeaderboardInfo> LeaderboardSystem::AllBoards() const {
    std::vector<LeaderboardInfo> out;
    for (auto& [k, b] : m_impl->boards) {
        LeaderboardInfo inf = b.info;
        inf.entryCount = static_cast<uint32_t>(b.entries.size());
        out.push_back(inf);
    }
    return out;
}

void LeaderboardSystem::SubmitScore(const std::string& boardName,
                                    const LeaderboardEntry& entry) {
    auto it = m_impl->boards.find(boardName);
    if (it == m_impl->boards.end()) return;
    Board& b = it->second;
    LeaderboardEntry e = entry;
    e.timestampMs = e.timestampMs ? e.timestampMs : NowMs();
    bool wasTop = !b.entries.empty() &&
                  (b.info.order == ScoreOrder::Descending
                   ? e.score > b.entries.front().score
                   : e.score < b.entries.front().score);
    b.entries.push_back(e);
    m_impl->Rank(b);

    if (wasTop && m_impl->onNewTop)
        m_impl->onNewTop(boardName, b.entries.front());
    if (m_impl->onRankChanged) {
        uint32_t rank = GetRank(boardName, e.playerId);
        m_impl->onRankChanged(boardName, e.playerId, rank);
    }
}

bool LeaderboardSystem::UpdateBestScore(const std::string& boardName,
                                        const LeaderboardEntry& entry) {
    auto it = m_impl->boards.find(boardName);
    if (it == m_impl->boards.end()) return false;
    Board& b = it->second;
    for (auto& e : b.entries) {
        if (e.playerId == entry.playerId) {
            bool better = b.info.order == ScoreOrder::Descending
                          ? entry.score > e.score : entry.score < e.score;
            if (!better) return false;
            e = entry; e.timestampMs = NowMs();
            m_impl->Rank(b);
            return true;
        }
    }
    SubmitScore(boardName, entry);
    return true;
}

std::vector<LeaderboardEntry> LeaderboardSystem::GetTopScores(
    const std::string& name, uint32_t count) const {
    auto it = m_impl->boards.find(name);
    if (it == m_impl->boards.end()) return {};
    auto& v = it->second.entries;
    uint32_t n = std::min(count, (uint32_t)v.size());
    return {v.begin(), v.begin() + n};
}

std::vector<LeaderboardEntry> LeaderboardSystem::GetAroundPlayer(
    const std::string& name, const std::string& playerId, uint32_t radius) const {
    auto it = m_impl->boards.find(name);
    if (it == m_impl->boards.end()) return {};
    auto& v = it->second.entries;
    for (size_t i = 0; i < v.size(); ++i) {
        if (v[i].playerId == playerId) {
            size_t lo = (size_t)(i > radius ? i - radius : 0);
            size_t hi = std::min(v.size(), i + radius + 1);
            return {v.begin() + lo, v.begin() + hi};
        }
    }
    return {};
}

uint32_t LeaderboardSystem::GetRank(const std::string& name,
                                    const std::string& playerId) const {
    auto it = m_impl->boards.find(name);
    if (it == m_impl->boards.end()) return 0;
    for (auto& e : it->second.entries)
        if (e.playerId == playerId) return e.rank;
    return 0;
}

LeaderboardEntry LeaderboardSystem::GetPlayerEntry(const std::string& name,
                                                   const std::string& pid) const {
    auto it = m_impl->boards.find(name);
    if (it == m_impl->boards.end()) return {};
    for (auto& e : it->second.entries)
        if (e.playerId == pid) return e;
    return {};
}

void LeaderboardSystem::Save() const {
    if (m_impl->persistPath.empty()) return;
    std::ofstream f(m_impl->persistPath);
    if (!f.is_open()) return;
    f << "{\"boards\":[";
    bool first = true;
    for (auto& [name, b] : m_impl->boards) {
        if (!first) f << ","; first = false;
        f << "{\"name\":\"" << name
          << "\",\"order\":" << static_cast<int>(b.info.order)
          << ",\"count\":" << b.entries.size() << "}";
    }
    f << "]}\n";
}

void LeaderboardSystem::Load() { /* reads persistPath JSON — stub */ }

void LeaderboardSystem::ResetBoard(const std::string& name) {
    auto it = m_impl->boards.find(name);
    if (it != m_impl->boards.end()) it->second.entries.clear();
}

void LeaderboardSystem::OnNewTopScore(
    std::function<void(const std::string&, const LeaderboardEntry&)> cb) {
    m_impl->onNewTop = std::move(cb);
}

void LeaderboardSystem::OnRankChanged(
    std::function<void(const std::string&, const std::string&, uint32_t)> cb) {
    m_impl->onRankChanged = std::move(cb);
}

} // namespace Runtime
