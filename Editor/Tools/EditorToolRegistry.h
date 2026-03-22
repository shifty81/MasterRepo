#pragma once
/**
 * @file EditorToolRegistry.h
 * @brief Tool lifecycle manager for the in-editor overlay tool system.
 *
 * Adapted from Nova-Forge-Expeditions (atlas::tools → Editor namespace).
 *
 * EditorToolRegistry owns ITool instances, handles activation/deactivation
 * (one active tool at a time), and drives per-frame Update():
 *
 *   ITool interface: Name(), Activate(), Deactivate(), Update(dt), IsActive().
 *
 *   EditorToolRegistry:
 *     - Register(tool): take ownership of an ITool.
 *     - Activate(name) / DeactivateCurrent()
 *     - ActiveTool(): currently active tool (nullptr if none)
 *     - UpdateActive(dt): call once per editor frame
 *     - Get(index) / FindByName(name)
 *     - ToolNames() / Count()
 *     - Clear(): remove all registered tools
 */

#include <memory>
#include <string>
#include <vector>

namespace Editor {

// ── ITool interface ───────────────────────────────────────────────────────
class ITool {
public:
    virtual ~ITool() = default;
    virtual std::string Name()      const = 0;
    virtual void        Activate()        = 0;
    virtual void        Deactivate()      = 0;
    virtual void        Update(float dt)  = 0;
    virtual bool        IsActive() const  = 0;
};

// ── Registry ──────────────────────────────────────────────────────────────
class EditorToolRegistry {
public:
    void Register(std::unique_ptr<ITool> tool);

    size_t Count() const;
    ITool* Get(size_t index)              const;
    ITool* FindByName(const std::string& name) const;

    bool   Activate(const std::string& name);
    void   DeactivateCurrent();
    ITool* ActiveTool() const;

    void UpdateActive(float dt);

    std::vector<std::string> ToolNames() const;
    void Clear();

private:
    std::vector<std::unique_ptr<ITool>> m_tools;
    ITool*                              m_active{nullptr};
};

} // namespace Editor
