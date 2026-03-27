#include "Core/Tags/TagSystem/TagSystem.h"
#include <algorithm>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Core {

static bool TagMatchesPattern(const std::string& entityTag,
                               const std::string& queryTag,
                               bool matchChildren)
{
    if (entityTag == queryTag) return true;
    if (matchChildren) {
        // "character.enemy" matches query "character" if entityTag starts with "character."
        if (entityTag.size() > queryTag.size() &&
            entityTag.substr(0, queryTag.size()) == queryTag &&
            entityTag[queryTag.size()] == '.')
            return true;
    }
    return false;
}

struct TagSystem::Impl {
    mutable std::mutex mu;
    std::unordered_map<uint32_t, std::unordered_set<std::string>> entityTags;
    std::unordered_map<std::string, TagGroupDesc> groups;

    struct SubEntry { uint32_t id; TagChangeCb cb; };
    std::vector<SubEntry> subs;
    uint32_t nextSubId{1};

    void Notify(const TagChangeEvent& evt) {
        for (auto& s : subs) if (s.cb) s.cb(evt);
    }

    bool EntityHasTag(uint32_t id, const std::string& tag, bool mc) const {
        auto it = entityTags.find(id);
        if (it == entityTags.end()) return false;
        for (auto& t : it->second) if (TagMatchesPattern(t, tag, mc)) return true;
        return false;
    }
};

TagSystem::TagSystem()  : m_impl(new Impl) {}
TagSystem::~TagSystem() { Shutdown(); delete m_impl; }

void TagSystem::Init()     {}
void TagSystem::Shutdown() { std::lock_guard<std::mutex> lk(m_impl->mu); m_impl->entityTags.clear(); }

void TagSystem::AddTag(uint32_t entityId, const std::string& tag)
{
    {
        std::lock_guard<std::mutex> lk(m_impl->mu);
        // Check exclusive groups
        for (auto& [gname, gd] : m_impl->groups) {
            bool tagInGroup = std::find(gd.exclusiveTags.begin(),
                                        gd.exclusiveTags.end(), tag)
                              != gd.exclusiveTags.end();
            if (tagInGroup) {
                // Remove any other exclusive tag from same group
                for (auto& excl : gd.exclusiveTags) {
                    if (excl != tag) m_impl->entityTags[entityId].erase(excl);
                }
            }
        }
        m_impl->entityTags[entityId].insert(tag);
    }
    m_impl->Notify({entityId, tag, true});
}

void TagSystem::RemoveTag(uint32_t entityId, const std::string& tag)
{
    {
        std::lock_guard<std::mutex> lk(m_impl->mu);
        auto it = m_impl->entityTags.find(entityId);
        if (it == m_impl->entityTags.end()) return;
        it->second.erase(tag);
    }
    m_impl->Notify({entityId, tag, false});
}

void TagSystem::RemoveAllTags(uint32_t entityId)
{
    std::vector<std::string> removed;
    {
        std::lock_guard<std::mutex> lk(m_impl->mu);
        auto it = m_impl->entityTags.find(entityId);
        if (it == m_impl->entityTags.end()) return;
        for (auto& t : it->second) removed.push_back(t);
        m_impl->entityTags.erase(it);
    }
    for (auto& t : removed) m_impl->Notify({entityId, t, false});
}

bool TagSystem::HasTag(uint32_t entityId, const std::string& tag, bool mc) const
{
    std::lock_guard<std::mutex> lk(m_impl->mu);
    return m_impl->EntityHasTag(entityId, tag, mc);
}

bool TagSystem::HasAllTags(uint32_t entityId, const std::vector<std::string>& tags, bool mc) const
{
    std::lock_guard<std::mutex> lk(m_impl->mu);
    for (auto& t : tags) if (!m_impl->EntityHasTag(entityId, t, mc)) return false;
    return true;
}

bool TagSystem::HasAnyTag(uint32_t entityId, const std::vector<std::string>& tags, bool mc) const
{
    std::lock_guard<std::mutex> lk(m_impl->mu);
    for (auto& t : tags) if (m_impl->EntityHasTag(entityId, t, mc)) return true;
    return false;
}

std::vector<std::string> TagSystem::GetTags(uint32_t entityId) const
{
    std::lock_guard<std::mutex> lk(m_impl->mu);
    auto it = m_impl->entityTags.find(entityId);
    if (it == m_impl->entityTags.end()) return {};
    return std::vector<std::string>(it->second.begin(), it->second.end());
}

void TagSystem::RegisterExclusiveGroup(const TagGroupDesc& desc)
{
    std::lock_guard<std::mutex> lk(m_impl->mu);
    m_impl->groups[desc.groupName] = desc;
}

void TagSystem::AddTagsToMany(const std::vector<uint32_t>& entities, const std::string& tag)
{
    for (auto id : entities) AddTag(id, tag);
}

void TagSystem::RemoveTagFromAll(const std::string& tag)
{
    std::vector<uint32_t> affected;
    {
        std::lock_guard<std::mutex> lk(m_impl->mu);
        for (auto& [id, tags] : m_impl->entityTags)
            if (tags.count(tag)) { tags.erase(tag); affected.push_back(id); }
    }
    for (auto id : affected) m_impl->Notify({id, tag, false});
}

std::vector<uint32_t> TagSystem::QueryAll(const std::vector<std::string>& require,
                                           const std::vector<std::string>& any,
                                           const std::vector<std::string>& exclude,
                                           bool mc) const
{
    std::lock_guard<std::mutex> lk(m_impl->mu);
    std::vector<uint32_t> out;
    for (auto& [id, tags] : m_impl->entityTags) {
        bool ok = true;
        for (auto& t : require) { if (!m_impl->EntityHasTag(id,t,mc)){ ok=false; break; } }
        if (!ok) continue;
        if (!any.empty()) {
            bool hasAny = false;
            for (auto& t : any) { if (m_impl->EntityHasTag(id,t,mc)){ hasAny=true; break; } }
            if (!hasAny) continue;
        }
        for (auto& t : exclude) { if (m_impl->EntityHasTag(id,t,mc)){ ok=false; break; } }
        if (!ok) continue;
        out.push_back(id);
    }
    return out;
}

uint32_t TagSystem::TaggedEntityCount() const
{
    std::lock_guard<std::mutex> lk(m_impl->mu);
    return (uint32_t)m_impl->entityTags.size();
}

uint32_t TagSystem::TagCount(const std::string& tag) const
{
    std::lock_guard<std::mutex> lk(m_impl->mu);
    uint32_t n = 0;
    for (auto& [id, tags] : m_impl->entityTags) if (tags.count(tag)) n++;
    return n;
}

uint32_t TagSystem::Subscribe(TagChangeCb cb)
{
    uint32_t id = m_impl->nextSubId++;
    m_impl->subs.push_back({id, cb});
    return id;
}

void TagSystem::Unsubscribe(uint32_t subId)
{
    auto& v = m_impl->subs;
    v.erase(std::remove_if(v.begin(),v.end(),[&](auto& s){ return s.id==subId; }), v.end());
}

} // namespace Core
