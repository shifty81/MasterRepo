#pragma once
#include "Renderer/RHI/RenderDevice.h"
#include "Renderer/Pipeline/ForwardRenderer.h"
#include "UI/Rendering/UIRenderer.h"
#include "Game/App/Orchestrator.h"
#include "Game/Voxel/ChunkMeshCache.h"
#include <cstdint>
#include <memory>

namespace NF::Game {

/// @brief Standalone game client application.
///
/// Creates a platform window, an OpenGL rendering context, and runs the full
/// Phase 3 interaction loop via the @c Orchestrator.  A minimal HUD is drawn
/// each frame using the custom @c UIRenderer.  Voxel chunk meshes are
/// rendered via @c ForwardRenderer and @c ChunkMeshCache (Phase 4).
///
/// On Windows a proper Win32 window is created.  On other platforms the game
/// loop runs headless (no window) for CI / headless smoke-test purposes.
class GameClientApp {
public:
    GameClientApp()  = default;
    ~GameClientApp() = default;

    GameClientApp(const GameClientApp&)            = delete;
    GameClientApp& operator=(const GameClientApp&) = delete;

    /// @brief Initialise the window, renderer, and game systems.
    /// @return True on success.
    bool Init();

    /// @brief Run the blocking game loop until the window is closed.
    void Run();

    /// @brief Tear down all subsystems.
    void Shutdown();

#ifdef _WIN32
    /// @brief Route a Win32 OS event into the client input state.
    void DispatchOsEvent(unsigned msg, uintptr_t wParam, intptr_t lParam) noexcept;
#endif

private:
    std::unique_ptr<RenderDevice> m_RenderDevice;
    ForwardRenderer               m_ForwardRenderer;
    UIRenderer                    m_UIRenderer;
    Orchestrator                  m_Orchestrator;
    ChunkMeshCache                m_MeshCache;

    bool  m_Running{false};
    void* m_Hwnd{nullptr};
    int   m_ClientWidth{1280};
    int   m_ClientHeight{720};
    float m_DpiScale{1.f};

    // Minimal input state (Win32 VK-indexed)
    bool m_Keys[256]{};
    bool m_KeysJustPressed[256]{};
    float m_MouseX{0.f};
    float m_MouseY{0.f};
    float m_MouseDeltaX{0.f};
    float m_MouseDeltaY{0.f};
    float m_PrevMouseX{0.f};
    float m_PrevMouseY{0.f};
    bool  m_LeftDown{false};
    bool  m_LeftJustPressed{false};
    bool  m_RightDown{false};
    bool  m_MouseTracking{false}; ///< True once we have a valid previous mouse pos.

    /// @brief Advance simulation and render one frame.
    void TickFrame(float dt);

    /// @brief Draw the in-game HUD (health, energy, tool, inventory).
    void DrawHUD();

    // Flush per-frame edge-triggered flags after all systems have read them.
    void FlushFrameInput() noexcept;

    /// @brief Compute the current view matrix from the player position and look.
    [[nodiscard]] NF::Matrix4x4 GetViewMatrix() const noexcept;

    /// @brief Compute the current projection matrix.
    [[nodiscard]] NF::Matrix4x4 GetProjectionMatrix() const noexcept;
};

} // namespace NF::Game
