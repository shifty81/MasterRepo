#include <iostream>
#include "Core/Metadata/Metadata.h"

int main() {
    Core::Metadata::MetadataStore store;
    store.Add("author", "engine-team", "info");
    store.Add("version", "1.0.0");
    store.Add("category", "core", "type");

    std::cout << "author: " << store.Get("author").value_or("<none>") << std::endl;
    std::cout << "version: " << store.Get("version").value_or("<none>") << std::endl;

    auto infoEntries = store.GetByCategory("info");
    std::cout << "Entries in 'info' category:" << std::endl;
    for (const auto& entry : infoEntries) {
        std::cout << "  " << entry.Key << ": " << entry.Value << std::endl;
    }

    store.Remove("author");
    std::cout << "author after remove: " << store.Get("author").value_or("<none>") << std::endl;
    return 0;
}
