#pragma once
#include <string>
#include <vector>
#include "Runtime/Inventory/Inventory.h"

namespace Runtime::Crafting {

struct Ingredient {
    std::string itemId;
    int         quantity = 1;
};

struct Recipe {
    std::string             id;
    std::string             name;
    std::string             outputItemId;
    int                     outputQuantity = 1;
    std::vector<Ingredient> ingredients;
    float                   craftTime = 1.0f; // seconds
    std::string             station;          // required crafting station type
};

class CraftingSystem {
public:
    void RegisterRecipe(const Recipe& recipe);

    const Recipe* FindRecipe(const std::string& id) const;

    // Returns pointers to every recipe for which `inv` contains sufficient
    // ingredients (ignoring station requirement for availability checks).
    std::vector<const Recipe*> GetAvailableRecipes(
        const Runtime::Inventory::Inventory& inv) const;

    bool CanCraft(const Recipe& recipe,
                  const Runtime::Inventory::Inventory& inv) const;

    // Consumes ingredients and adds the output item to `inv`.
    // Returns false if ingredients are missing or the output cannot be added.
    bool Craft(const Recipe& recipe, Runtime::Inventory::Inventory& inv);

    const std::vector<Recipe>& GetAllRecipes() const { return m_recipes; }

private:
    std::vector<Recipe> m_recipes;
};

} // namespace Runtime::Crafting
