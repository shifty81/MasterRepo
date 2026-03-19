#include "Farming.h"

int FarmingSystem::GetGrowthDays(CropType type) {
    switch (type) {
        case CropType::PARSNIP: return 4;
        case CropType::POTATO:  return 6;
        case CropType::TOMATO:  return 8;
        default: return 5;
    }
}

std::string FarmingSystem::GetCropName(CropType type) {
    switch (type) {
        case CropType::PARSNIP: return "Parsnip";
        case CropType::POTATO:  return "Potato";
        case CropType::TOMATO:  return "Tomato";
        default: return "Unknown";
    }
}

int FarmingSystem::GetCropValue(CropType type) {
    switch (type) {
        case CropType::PARSNIP: return 35;
        case CropType::POTATO:  return 80;
        case CropType::TOMATO:  return 60;
        default: return 25;
    }
}
