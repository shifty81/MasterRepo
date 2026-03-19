#include "Runtime/Crafting/Crafting.h"
#include <algorithm>

namespace Runtime::Crafting {

void CraftingSystem::RegisterRecipe(const Recipe& recipe) {
    // Replace existing recipe with the same id, or append.
    for (auto& r : m_recipes) {
        if (r.id == recipe.id) {
            r = recipe;
            return;
        }
    }
    m_recipes.push_back(recipe);
}

const Recipe* CraftingSystem::FindRecipe(const std::string& id) const {
    for (const auto& r : m_recipes)
        if (r.id == id)
            return &r;
    return nullptr;
}

bool CraftingSystem::CanCraft(const Recipe& recipe,
                               const Runtime::Inventory::Inventory& inv) const {
    for (const auto& ingredient : recipe.ingredients) {
        int total = 0;
        for (const auto& item : inv.GetItems())
            if (item.id == ingredient.itemId)
                total += item.quantity;
        if (total < ingredient.quantity)
            return false;
    }
    return true;
}

std::vector<const Recipe*> CraftingSystem::GetAvailableRecipes(
    const Runtime::Inventory::Inventory& inv) const
{
    std::vector<const Recipe*> result;
    for (const auto& r : m_recipes)
        if (CanCraft(r, inv))
            result.push_back(&r);
    return result;
}

bool CraftingSystem::Craft(const Recipe& recipe,
                            Runtime::Inventory::Inventory& inv) {
    if (!CanCraft(recipe, inv))
        return false;

    // Consume ingredients.
    for (const auto& ingredient : recipe.ingredients) {
        if (!inv.RemoveItem(ingredient.itemId, ingredient.quantity))
            return false; // shouldn't happen after CanCraft, but be safe
    }

    // Produce output item.  We look up the first matching item for metadata;
    // if not found we create a minimal placeholder with just the id.
    Runtime::Inventory::Item output;
    output.id       = recipe.outputItemId;
    output.name     = recipe.outputItemId;
    output.quantity = recipe.outputQuantity;

    return inv.AddItem(output);
}

} // namespace Runtime::Crafting
