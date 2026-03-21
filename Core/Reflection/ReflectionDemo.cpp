#include <iostream>
#include "Core/Reflection/Person.h"
#include "Core/Reflection/Reflect.h"

int main() {
    Core::Person p{"Alice", 30, 1.65f};
    std::cout << "Properties of Core::Person:" << std::endl;
    Core::ForEachProperty("Core::Person", [&](const Core::Property& prop) {
        std::cout << "  " << prop.Name << " (" << prop.TypeName << ")";
        if (prop.TypeName == "int") {
            int value = 0;
            prop.Getter(&p, &value);
            std::cout << " = " << value;
        } else if (prop.TypeName == "float") {
            float value = 0.0f;
            prop.Getter(&p, &value);
            std::cout << " = " << value;
        } else if (prop.TypeName == "std::string") {
            std::string value;
            prop.Getter(&p, &value);
            std::cout << " = " << value;
        }
        std::cout << std::endl;
    });
    return 0;
}
