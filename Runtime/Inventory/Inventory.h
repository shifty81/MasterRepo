#pragma once
#include <string>
#include <vector>
#include <algorithm>

namespace Runtime::Inventory {

struct Item {
    std::string id;
    std::string name;
    std::string category;
    int         quantity = 1;
    int         maxStack = 64;
    float       weight   = 0.1f;
    std::string iconPath;
};

class Inventory {
public:
    explicit Inventory(int capacity = 40);

    // Returns true if the item was fully added.  Stacks if an identical id
    // already exists and there is room in the stack.
    bool AddItem(const Item& item);

    // Removes `quantity` of itemId.  Returns true if successful.
    bool RemoveItem(const std::string& id, int quantity = 1);

    Item*       FindItem(const std::string& id);
    const Item* FindItem(const std::string& id) const;

    std::vector<Item> GetItemsByCategory(const std::string& category) const;

    int   GetUsedSlots()   const;
    int   GetCapacity()    const { return m_capacity; }
    float GetTotalWeight() const;
    void  Clear();

    const std::vector<Item>& GetItems() const { return m_items; }

private:
    int              m_capacity;
    std::vector<Item> m_items;
};

} // namespace Runtime::Inventory
