#pragma once
/**
 * @file TagSystem.h
 * @brief Hierarchical entity tag system with fast query and inheritance.
 *
 * Features:
 *   - Hierarchical tags using dot-notation: "character.player", "item.weapon.sword"
 *   - Add/remove/query tags on entities (uint32_t IDs)
 *   - Parent-tag inheritance queries: HasTag("character") matches "character.player"
 *   - Exclusive tag groups (e.g. only one state tag active at a time)
 *   - Tag change callbacks
 *   - Batch operations: AddTagsToMany, RemoveTagsFromAll
 *   - Query: entities with ALL/ANY/NONE of a tag set
 *   - Thread-safe reads; writes protected by a mutex
 *
 * Typical usage:
 * @code
 *   TagSystem ts;
 *   ts.Init();
 *   ts.AddTag(entityId, "character.enemy.grunt");
 *   ts.AddTag(entityId, "state.patrol");
 *   if (ts.HasTag(entityId, "character.enemy")) { /* is any enemy * / }
 *   auto enemies = ts.QueryAll({"character.enemy"}, {}, {});
 *   ts.SetOnTagChanged([](uint32_t id, const std::string& tag, bool added){});
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Core {

struct TagGroupDesc {
    std::string              groupName;
    std::vector<std::string> exclusiveTags; ///< at most one active at a time
};

struct TagChangeEvent {
    uint32_t    entityId{0};
    std::string tag;
    bool        added{true};
};

class TagSystem {
public:
    TagSystem();
    ~TagSystem();

    void Init();
    void Shutdown();

    // Entity tags
    void AddTag(uint32_t entityId, const std::string& tag);
    void RemoveTag(uint32_t entityId, const std::string& tag);
    void RemoveAllTags(uint32_t entityId);
    bool HasTag(uint32_t entityId, const std::string& tag,
                bool matchChildren=true) const;
    bool HasAllTags(uint32_t entityId, const std::vector<std::string>& tags,
                    bool matchChildren=true) const;
    bool HasAnyTag(uint32_t entityId, const std::vector<std::string>& tags,
                   bool matchChildren=true) const;
    std::vector<std::string> GetTags(uint32_t entityId) const;

    // Exclusive groups
    void RegisterExclusiveGroup(const TagGroupDesc& desc);

    // Batch ops
    void AddTagsToMany(const std::vector<uint32_t>& entities, const std::string& tag);
    void RemoveTagFromAll(const std::string& tag);

    // Queries (return matching entity IDs)
    std::vector<uint32_t> QueryAll(const std::vector<std::string>& require,
                                   const std::vector<std::string>& any,
                                   const std::vector<std::string>& exclude,
                                   bool matchChildren=true) const;

    // Stats
    uint32_t TaggedEntityCount() const;
    uint32_t TagCount(const std::string& tag) const;

    // Callbacks
    using TagChangeCb = std::function<void(const TagChangeEvent&)>;
    uint32_t Subscribe(TagChangeCb cb);
    void     Unsubscribe(uint32_t subId);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Core
