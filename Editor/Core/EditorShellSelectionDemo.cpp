#include "Editor/Core/EditorShell.h"
#include "Engine/Math/Math.h"
#include <iostream>

int main() {
    Editor::EditorShell shell;
    shell.AddPanel(std::make_unique<Editor::ViewportPanel>(&shell));
    shell.AddPanel(std::make_unique<Editor::InspectorPanel>(&shell));
    // Simulate editor loop
    for (int i = 0; i < 3; ++i) {
        std::cout << "--- Frame " << i << " ---\n";
        shell.Draw();
    }
    return 0;
}
