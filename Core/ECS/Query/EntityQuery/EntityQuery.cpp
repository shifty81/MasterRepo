#include "Core/ECS/Query/EntityQuery/EntityQuery.h"
#include <unordered_map>
#include <unordered_set>

namespace Core {

struct QueryState {
    std::unordered_set<uint32_t> required;
    std::vector<uint32_t>        results;
    bool                         dirty{true};
    std::function<void(uint32_t)> onRebuild;
};

struct EntityQuery::Impl {
    std::unordered_map<uint32_t, QueryState> queries;
};

EntityQuery::EntityQuery()  { m_impl = new Impl; }
EntityQuery::~EntityQuery() { delete m_impl; }
void EntityQuery::Init    () {}
void EntityQuery::Shutdown() { Reset(); }
void EntityQuery::Reset   () { m_impl->queries.clear(); }

bool EntityQuery::CreateQuery(uint32_t qid, const std::vector<uint32_t>& required) {
    if (m_impl->queries.count(qid)) return false;
    QueryState s; s.required.insert(required.begin(), required.end());
    m_impl->queries[qid] = std::move(s);
    return true;
}
void EntityQuery::DestroyQuery(uint32_t qid) { m_impl->queries.erase(qid); }

void EntityQuery::AddRequiredType(uint32_t qid, uint32_t typeId) {
    auto it = m_impl->queries.find(qid);
    if (it != m_impl->queries.end()) { it->second.required.insert(typeId); it->second.dirty = true; }
}
void EntityQuery::RemoveRequiredType(uint32_t qid, uint32_t typeId) {
    auto it = m_impl->queries.find(qid);
    if (it != m_impl->queries.end()) { it->second.required.erase(typeId); it->second.dirty = true; }
}

void EntityQuery::Rebuild(const std::vector<uint32_t>& worldEntities,
                           std::function<bool(uint32_t, uint32_t)> hasComponent) {
    for (auto& [qid, qs] : m_impl->queries) {
        qs.results.clear();
        for (uint32_t eid : worldEntities) {
            bool match = true;
            for (uint32_t tid : qs.required)
                if (!hasComponent(eid, tid)) { match = false; break; }
            if (match) qs.results.push_back(eid);
        }
        qs.dirty = false;
        if (qs.onRebuild) qs.onRebuild((uint32_t)qs.results.size());
    }
}

void EntityQuery::MarkDirty(uint32_t qid) {
    auto it = m_impl->queries.find(qid); if (it != m_impl->queries.end()) it->second.dirty = true;
}
bool EntityQuery::IsStale(uint32_t qid) const {
    auto it = m_impl->queries.find(qid); return it != m_impl->queries.end() && it->second.dirty;
}
uint32_t EntityQuery::GetResults(uint32_t qid, std::vector<uint32_t>& out) const {
    auto it = m_impl->queries.find(qid); if (it == m_impl->queries.end()) return 0;
    out = it->second.results; return (uint32_t)out.size();
}
uint32_t EntityQuery::GetResultCount(uint32_t qid) const {
    auto it = m_impl->queries.find(qid); return it != m_impl->queries.end() ? (uint32_t)it->second.results.size() : 0;
}
void EntityQuery::ForEach(uint32_t qid, std::function<void(uint32_t)> fn) const {
    auto it = m_impl->queries.find(qid); if (it == m_impl->queries.end() || !fn) return;
    for (uint32_t eid : it->second.results) fn(eid);
}
void EntityQuery::SetOnRebuild(uint32_t qid, std::function<void(uint32_t)> cb) {
    auto it = m_impl->queries.find(qid); if (it != m_impl->queries.end()) it->second.onRebuild = std::move(cb);
}

} // namespace Core
