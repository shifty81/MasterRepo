#pragma once
/**
 * @file InventoryFilter.h
 * @brief Predicate-based inventory search, sort, and filter pipeline over item collections.
 *
 * Features:
 *   - AddItem(item): register an item in the filter's working set
 *   - RemoveItem(id)
 *   - AddPredicate(name, fn): register a named filter predicate (item → bool)
 *   - RemovePredicate(name)
 *   - Apply(predicates[]) → ids[]: execute listed predicates AND-chained, return matching item ids
 *   - ApplyAll() → ids[]: run all registered predicates
 *   - Sort(field, ascending) → ids[]: sort current result by "name"/"weight"/"quantity"/"rarity"
 *   - Search(text) → ids[]: substring match on item name
 *   - SetRarityFilter(minRarity, maxRarity)
 *   - SetWeightLimit(maxWeight): filter items exceeding weight
 *   - SetTagFilter(tags[]): only items having ALL specified tags
 *   - GetResult() → ids[]: last computed result list
 *   - ClearFilters()
 *   - GetItemCount() → uint32_t
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

struct FilterItem {
    uint32_t    id{0};
    std::string name;
    float       weight{0.f};
    uint32_t    quantity{1};
    uint32_t    rarity{0};    ///< 0=common…4=legendary
    std::vector<std::string> tags;
};

class InventoryFilter {
public:
    InventoryFilter();
    ~InventoryFilter();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Item management
    void AddItem   (const FilterItem& item);
    void RemoveItem(uint32_t id);
    void ClearItems();

    // Predicate registration
    void AddPredicate   (const std::string& name, std::function<bool(const FilterItem&)> fn);
    void RemovePredicate(const std::string& name);

    // Filter operations
    std::vector<uint32_t> Apply   (const std::vector<std::string>& predicateNames) const;
    std::vector<uint32_t> ApplyAll() const;

    // Convenience filters
    void SetRarityFilter(uint32_t minRarity, uint32_t maxRarity);
    void SetWeightLimit (float maxWeight);
    void SetTagFilter   (const std::vector<std::string>& tags);

    // Search & sort
    std::vector<uint32_t> Search(const std::string& text) const;
    std::vector<uint32_t> Sort  (const std::string& field, bool ascending=true) const;

    // Result accessor (last Apply/Search/Sort result)
    const std::vector<uint32_t>& GetResult() const;
    void                         ClearFilters();

    uint32_t GetItemCount() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
