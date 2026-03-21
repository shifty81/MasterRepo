#include "Editor/Core/EditorShell.h"
#include <iostream>

struct DummyPanel : public Editor::Panel {
    DummyPanel(const std::string& n) { name = n; }
    void Draw() override { std::cout << "Drawing panel: " << name << std::endl; }
};

int main() {
    Editor::EditorShell shell;
    shell.AddPanel(std::make_unique<Editor::ViewportPanel>());
    shell.AddPanel(std::make_unique<Editor::InspectorPanel>());
    shell.AddPanel(std::make_unique<DummyPanel>("Hierarchy"));
    shell.Draw();
    return 0;
}
