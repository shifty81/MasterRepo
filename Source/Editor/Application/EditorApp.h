#pragma once
#include "Renderer/RHI/RenderDevice.h"
#include "Renderer/Pipeline/ForwardRenderer.h"
#include "UI/Rendering/UIRenderer.h"
#include "Engine/ECS/World.h"
#include "Engine/World/Level.h"
#include "Game/World/GameWorld.h"
#include "Game/Interaction/InteractionLoop.h"
#include "Game/Voxel/ChunkMeshCache.h"
#include "Editor/Application/EditorInputState.h"
#include "Editor/Panels/DockingSystem.h"
#include "Editor/Panels/SceneOutliner.h"
#include "Editor/Panels/Inspector.h"
#include "Editor/Panels/ContentBrowser.h"
#include "Editor/Panels/ConsolePanel.h"
#include "Editor/Panels/VoxelInspector.h"
#include "Editor/Panels/HUDPanel.h"
#include "Editor/Panels/EditorToolbar.h"
#include "Editor/Panels/MenuBarPanel.h"
#include "Editor/Commands/EditorCommandRegistry.h"
#include "Editor/Viewport/EditorViewport.h"
#include <cstdint>
#include <memory>

namespace NF::Editor {

/// @brief Top-level editor application; owns the device, world and level.
class EditorApp {
public:
    /// @brief Initialise all editor subsystems.
    /// @return True on success.
    bool Init();

    /// @brief Run the blocking editor loop.
    void Run();

    /// @brief Tear down all editor subsystems.
    void Shutdown();

    /// @brief Return a pointer to the render device.
    [[nodiscard]] RenderDevice* GetRenderDevice() noexcept { return m_RenderDevice.get(); }

    /// @brief Return a pointer to the active ECS world.
    [[nodiscard]] World* GetWorld() noexcept { return &m_Level.GetWorld(); }

#ifdef _WIN32
    /// @brief Route a Win32 OS event into editor input state.
    /// Called by the Win32 window procedure on the message-dispatch thread.
    void DispatchOsEvent(unsigned msg, uintptr_t wParam, intptr_t lParam) noexcept;
#endif

private:
    std::unique_ptr<RenderDevice> m_RenderDevice;
    ForwardRenderer               m_ForwardRenderer;
    UIRenderer                    m_UIRenderer;
    NF::Game::GameWorld           m_GameWorld;
    NF::Game::InteractionLoop     m_InteractionLoop;
    NF::Game::ChunkMeshCache      m_MeshCache;
    Level                         m_Level;
    bool                          m_Running{false};
    void*                         m_Hwnd{nullptr};
    int                           m_ClientWidth{1280};
    int                           m_ClientHeight{720};

    EditorInputState              m_Input;          ///< Per-frame OS input state.
    float                         m_DpiScale{1.f};  ///< Monitor DPI / 96.

    /// @brief Advance all panels and issue one frame of rendering.
    /// @param dt Elapsed seconds since the previous frame.
    void TickFrame(float dt);

    /// @brief Register all built-in editor commands.
    void RegisterEditorCommands();

    /// @brief Draw the menu bar.
    void DrawMenuBar(float x, float y, float w, float h);

    DockingSystem  m_DockingSystem;
    SceneOutliner  m_SceneOutliner;
    Inspector      m_Inspector;
    ContentBrowser m_ContentBrowser;
    ConsolePanel   m_ConsolePanel;
    EditorViewport m_Viewport;
    VoxelInspector m_VoxelInspector;
    HUDPanel       m_HUDPanel;
    EditorToolbar  m_Toolbar;

    // Command system and menu bar
    nf::EditorCommandRegistry m_CommandRegistry;
    nf::MenuBarPanel          m_MenuBar;
};

} // namespace NF::Editor
