#pragma once
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include "Core/Reflection/Reflect.h"

namespace Editor {

struct Panel {
    std::string name;
    bool visible = true;
    virtual void Draw() = 0;
    virtual ~Panel() = default;
};

// Forward declaration for selection
struct InspectorPanel;

struct EditorShell {
    std::vector<std::unique_ptr<Panel>> panels;
    void* selectedObject = nullptr;
    std::string selectedType;

    void AddPanel(std::unique_ptr<Panel> panel) { panels.push_back(std::move(panel)); }
    void Draw() {
        for (auto& panel : panels) {
            if (panel->visible) panel->Draw();
        }
    }
    void Select(void* obj, const std::string& typeName) {
        selectedObject = obj;
        selectedType = typeName;
    }
};

struct ViewportPanel : public Panel {
    EditorShell* shell = nullptr;
    ViewportPanel(EditorShell* s) { name = "Viewport"; shell = s; }
    void Draw() override {
        std::cout << "[Viewport] Click to select object...\n";
        // Simulate selection (in real UI, would be mouse picking)
        static bool selected = false;
        if (!selected && shell) {
            // Simulate selecting a Vec3 object
            static Engine::Math::Vec3 dummyObj{1,2,3};
            shell->Select(&dummyObj, "Engine::Math::Vec3");
            selected = true;
            std::cout << "[Viewport] Selected Vec3 object.\n";
        }
    }
};

struct InspectorPanel : public Panel {
    EditorShell* shell = nullptr;
    InspectorPanel(EditorShell* s) { name = "Inspector"; shell = s; }
    void Draw() override {
        std::cout << "[Inspector] Properties:\n";
        if (shell && shell->selectedObject && !shell->selectedType.empty()) {
            Core::ForEachProperty(shell->selectedType, [&](const Core::Property& prop) {
                std::cout << "  " << prop.Name << " (" << prop.TypeName << ")";
                if (prop.TypeName == "float") {
                    float value = 0.0f;
                    prop.Getter(shell->selectedObject, &value);
                    std::cout << " = " << value;
                } else if (prop.TypeName == "int") {
                    int value = 0;
                    prop.Getter(shell->selectedObject, &value);
                    std::cout << " = " << value;
                }
                std::cout << std::endl;
            });
        } else {
            std::cout << "  (No object selected)\n";
        }
    }
};

} // namespace Editor
