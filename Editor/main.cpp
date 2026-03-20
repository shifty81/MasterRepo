#include "Engine/Core/Engine.h"
#include "Editor/UI/EditorLayout.h"
#include "Editor/Viewport/ViewportPanel.h"
#include "Editor/Panels/Console/ConsolePanel.h"
#include "Editor/Modes/EditorModes.h"
#include "Editor/NodeEditors/NodeEditor.h"
#include "Editor/Gizmos/GizmoSystem.h"
#include "Editor/Commands/EditorCommandBus.h"
#include "Editor/Panels/SceneOutliner/LayerTagSystem.h"
#include "Editor/Panels/Inspector/InspectorTools.h"
#include "Editor/Panels/ContentBrowser/ContentBrowser.h"
#include <iostream>
#include <memory>
#include <filesystem>

int main() {
    std::cout << "[Editor] Working directory: "
              << std::filesystem::current_path().string() << "\n";

    // ── Engine setup ────────────────────────────────────────────────────────
    Engine::Core::EngineConfig cfg;
    cfg.mode = Engine::Core::EngineMode::Editor;

    Engine::Core::Engine engine(cfg);
    engine.InitCore();
    engine.InitRender();
    engine.InitUI();
    engine.InitECS();

    // ── Viewport & console ──────────────────────────────────────────────────
    Editor::ViewportPanel viewport;
    Editor::ConsolePanel  console;

    // ── Editor mode controller ──────────────────────────────────────────────
    Editor::EditorModeController modes;
    modes.SetOnModeChanged([](Editor::EditorModeType prev, Editor::EditorModeType next) {
        (void)prev; (void)next;
    });

    // ── Node editor ─────────────────────────────────────────────────────────
    Editor::NodeEditor nodeEditor;

    // ── Gizmos & overlays ───────────────────────────────────────────────────
    Editor::GizmoSystem   gizmos;
    Editor::OverlaySystem overlaySystem;

    gizmos.SetGizmoType(Editor::GizmoType::Translate);
    viewport.SetGizmoMode(Editor::GizmoMode::Translate);

    // Stats overlays for in-editor diagnostics
    overlaySystem.SetFPS(0.0f);
    overlaySystem.SetEntityCount(0);

    // ── Command bus ─────────────────────────────────────────────────────────
    Editor::EditorCommandBus cmdBus;
    // SelectEntity: forward the entity ID into the gizmo system so it
    // tracks the selected object's transform.
    cmdBus.RegisterHandler(Editor::EditorCommandType::SelectEntity,
        [&gizmos](const Editor::EditorCommand& cmd) {
            float pos[3] = {0.f, 0.f, 0.f};
            float rot[4] = {0.f, 0.f, 0.f, 1.f};
            float scl[3] = {1.f, 1.f, 1.f};
            gizmos.SelectEntity(cmd.entityId, pos, rot, scl);
        });

    // ── Layer / tag system ──────────────────────────────────────────────────
    Editor::LayerTagSystem layers;
    layers.CreateLayer("Default");
    layers.CreateLayer("Background");

    // ── Inspector & material override ────────────────────────────────────────
    Editor::EntityInspectorTool inspector;
    inspector.Activate();

    // ── Content browser ──────────────────────────────────────────────────────
    Editor::ContentBrowser contentBrowser;
    contentBrowser.SetRootPath("assets");

    // ── Editor layout (dock tree, menu bar) ──────────────────────────────────
    Editor::EditorLayout layout;
    layout.GetMenuBar().onNewScene  = []{ std::cout << "[Editor] New scene\n"; };
    layout.GetMenuBar().onSaveScene = []{ std::cout << "[Editor] Save scene\n"; };
    layout.GetMenuBar().onOpenScene = []{ std::cout << "[Editor] Open scene\n"; };

    console.AddLine("[Editor] initialized with all systems");
    std::cout << "[Editor] initialized with all systems\n";

    engine.Run();
    return 0;
}
