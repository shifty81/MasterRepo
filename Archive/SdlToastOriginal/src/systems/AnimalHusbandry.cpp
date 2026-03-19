#include "AnimalHusbandry.h"
#include <algorithm>

AnimalHusbandrySystem::AnimalHusbandrySystem() {}

int AnimalHusbandrySystem::AddAnimal(const std::string& name, AnimalType type) {
    if (name.empty()) return -1;
    if (type == AnimalType::COUNT) return -1;

    Animal animal;
    animal.name = name;
    animal.type = type;
    animal.happiness = 5;  // Start at neutral happiness
    animal.daysOwned = 0;
    animal.fedToday = false;
    animal.pettedToday = false;

    m_animals.push_back(animal);
    return static_cast<int>(m_animals.size()) - 1;
}

bool AnimalHusbandrySystem::RemoveAnimal(int index) {
    if (index < 0 || index >= static_cast<int>(m_animals.size())) return false;
    m_animals.erase(m_animals.begin() + index);
    return true;
}

Animal* AnimalHusbandrySystem::GetAnimal(int index) {
    if (index < 0 || index >= static_cast<int>(m_animals.size())) return nullptr;
    return &m_animals[index];
}

const Animal* AnimalHusbandrySystem::GetAnimal(int index) const {
    if (index < 0 || index >= static_cast<int>(m_animals.size())) return nullptr;
    return &m_animals[index];
}

bool AnimalHusbandrySystem::FeedAnimal(int index) {
    Animal* animal = GetAnimal(index);
    if (!animal) return false;
    if (animal->fedToday) return false;  // Already fed

    animal->fedToday = true;
    return true;
}

bool AnimalHusbandrySystem::PetAnimal(int index) {
    Animal* animal = GetAnimal(index);
    if (!animal) return false;
    if (animal->pettedToday) return false;  // Already petted

    animal->pettedToday = true;
    return true;
}

void AnimalHusbandrySystem::AdvanceDay() {
    for (auto& animal : m_animals) {
        animal.daysOwned++;

        // Happiness changes based on care
        if (animal.fedToday && animal.pettedToday) {
            animal.happiness = std::min(animal.happiness + 1, MAX_HAPPINESS);
        } else if (!animal.fedToday) {
            animal.happiness = std::max(animal.happiness - 2, 0);
        } else if (!animal.pettedToday) {
            // Fed but not petted: no change
        }

        // Reset daily flags
        animal.fedToday = false;
        animal.pettedToday = false;
    }
}

bool AnimalHusbandrySystem::CanProduce(int index) const {
    const Animal* animal = GetAnimal(index);
    if (!animal) return false;
    // Must be old enough and reasonably happy
    return animal->daysOwned >= MIN_PRODUCE_DAYS && animal->happiness >= 3;
}

AnimalProduct AnimalHusbandrySystem::GetProduct(AnimalType type) const {
    switch (type) {
        case AnimalType::CHICKEN: return {"Egg", 25};
        case AnimalType::COW:     return {"Milk", 50};
        case AnimalType::SHEEP:   return {"Wool", 75};
        case AnimalType::GOAT:    return {"Goat Milk", 60};
        case AnimalType::PIG:     return {"Truffle", 100};
        default:                  return {"Unknown", 0};
    }
}

int AnimalHusbandrySystem::GetProductValue(int index) const {
    const Animal* animal = GetAnimal(index);
    if (!animal) return 0;
    if (!CanProduce(index)) return 0;

    AnimalProduct product = GetProduct(animal->type);
    // Happiness multiplier: 0.5x at happiness 3, up to 2.0x at happiness 10
    // Formula: baseValue * (0.5 + happiness * 0.15)
    float multiplier = 0.5f + animal->happiness * 0.15f;
    return static_cast<int>(product.baseValue * multiplier);
}

int AnimalHusbandrySystem::GetPurchaseCost(AnimalType type) {
    switch (type) {
        case AnimalType::CHICKEN: return 200;
        case AnimalType::COW:     return 500;
        case AnimalType::SHEEP:   return 400;
        case AnimalType::GOAT:    return 450;
        case AnimalType::PIG:     return 600;
        default:                  return 0;
    }
}

std::string AnimalHusbandrySystem::GetAnimalTypeName(AnimalType type) {
    switch (type) {
        case AnimalType::CHICKEN: return "Chicken";
        case AnimalType::COW:     return "Cow";
        case AnimalType::SHEEP:   return "Sheep";
        case AnimalType::GOAT:    return "Goat";
        case AnimalType::PIG:     return "Pig";
        default:                  return "Unknown";
    }
}

int AnimalHusbandrySystem::GetFeedEnergyCost() {
    return FEED_ENERGY_COST;
}
