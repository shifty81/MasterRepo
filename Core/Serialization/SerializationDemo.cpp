#include <iostream>
#include "Core/Serialization/Serializer.h"

int main() {
    using namespace Core::Serialization;
    // Create a JSON object
    JsonValue obj(JsonValue::ObjectType{
        {"name", "Atlas"},
        {"version", 1},
        {"features", JsonValue::ArrayType{ "reflection", "events", "serialization" }}
    });

    // Serialize to string
    std::string json = JsonSerializer::SerializeToPrettyString(obj);
    std::cout << "Serialized JSON:\n" << json << std::endl;

    // Parse back
    JsonValue parsed = JsonSerializer::DeserializeFromString(json);
    std::cout << "Parsed name: " << parsed["name"].AsString() << std::endl;
    std::cout << "Parsed version: " << parsed["version"].AsInt() << std::endl;
    std::cout << "Parsed features:";
    for (const auto& feat : parsed["features"].AsArray()) {
        std::cout << " " << feat.AsString();
    }
    std::cout << std::endl;
    return 0;
}
