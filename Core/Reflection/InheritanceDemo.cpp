#include <iostream>
#include "Core/Reflection/EntityPlayer.h"
#include "Core/Reflection/Reflect.h"

int main() {
    Core::Player player;
    player.id = 42;
    player.tag = "hero";
    player.name = "Eve";
    player.level = 7;

    std::cout << "Properties of Core::Player (including base):" << std::endl;
    Core::ForEachProperty("Core::Player", [&](const Core::Property& prop) {
        std::cout << "  " << prop.Name << " (" << prop.TypeName << ")";
        if (prop.TypeName == "int") {
            int value = 0;
            prop.Getter(&player, &value);
            std::cout << " = " << value;
        } else if (prop.TypeName == "std::string") {
            std::string value;
            prop.Getter(&player, &value);
            std::cout << " = " << value;
        }
        std::cout << std::endl;
    });
    // Test property lookup (should find base class property)
    const auto* info = Core::TypeRegistry::Instance().Find("Core::Player");
    if (info) {
        const auto* prop = info->FindProperty("tag");
        if (prop) {
            std::string tagValue;
            prop->Getter(&player, &tagValue);
            std::cout << "Lookup: tag = " << tagValue << std::endl;
        }
    }
    return 0;
}
