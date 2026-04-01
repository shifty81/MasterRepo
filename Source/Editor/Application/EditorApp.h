#pragma once
#include "Renderer/RHI/RenderDevice.h"
#include "Engine/ECS/World.h"
#include "Engine/World/Level.h"
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

private:
    std::unique_ptr<RenderDevice> m_RenderDevice;
    Level                         m_Level;
    bool                          m_Running{false};
};

} // namespace NF::Editor
