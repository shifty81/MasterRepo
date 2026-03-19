#include "Crafting.h"
#include "Inventory.h"

Crafting::Crafting() {
    InitRecipes();
}

void Crafting::InitRecipes() {
    // Recipe 0: Wood x5 -> Fence x1
    m_recipes.push_back({"Fence", 1, {{"Wood", 5}}});

    // Recipe 1: Wood x10 -> Chest x1
    m_recipes.push_back({"Chest", 1, {{"Wood", 10}}});

    // Recipe 2: Wood x3 + Stone x3 -> Sprinkler x1
    m_recipes.push_back({"Sprinkler", 1, {{"Wood", 3}, {"Stone", 3}}});

    // Recipe 3: Parsnip x3 -> Parsnip Soup x1
    m_recipes.push_back({"Parsnip Soup", 1, {{"Parsnip", 3}}});

    // Recipe 4: Potato x2 + Wood x1 -> Baked Potato x1
    m_recipes.push_back({"Baked Potato", 1, {{"Potato", 2}, {"Wood", 1}}});

    // Recipe 5: Tomato x3 -> Tomato Sauce x1
    m_recipes.push_back({"Tomato Sauce", 1, {{"Tomato", 3}}});

    // Recipe 6: Stone x5 -> Stone Wall x1
    m_recipes.push_back({"Stone Wall", 1, {{"Stone", 5}}});

    // Recipe 7: Wood x20 + Stone x10 -> Bridge x1
    m_recipes.push_back({"Bridge", 1, {{"Wood", 20}, {"Stone", 10}}});
}

bool Crafting::CanCraft(int recipeIndex, const Inventory* inventory) const {
    if (recipeIndex < 0 || recipeIndex >= static_cast<int>(m_recipes.size())) return false;
    if (!inventory) return false;

    const CraftingRecipe& recipe = m_recipes[recipeIndex];
    for (const auto& ingredient : recipe.ingredients) {
        if (inventory->GetItemCount(ingredient.name) < ingredient.quantity) {
            return false;
        }
    }
    return true;
}

bool Crafting::Craft(int recipeIndex, Inventory* inventory) {
    if (!CanCraft(recipeIndex, inventory)) return false;

    const CraftingRecipe& recipe = m_recipes[recipeIndex];

    // Remove ingredients
    for (const auto& ingredient : recipe.ingredients) {
        inventory->RemoveItem(ingredient.name, ingredient.quantity);
    }

    // Add result
    inventory->AddItem(recipe.resultName, recipe.resultQuantity);
    return true;
}
