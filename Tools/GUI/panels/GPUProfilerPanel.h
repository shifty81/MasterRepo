#pragma once
#include <string>
#include "Tools/GUI/IToolPanel.h"

namespace Tools {
class GPUProfiler;
}

namespace Tools {

/**
 * GPUProfilerPanel — displays GPU frame timing and zone breakdown.
 * Uses a GPUProfiler* set via SetProfiler().
 */
class GPUProfilerPanel : public IToolPanel {
public:
    GPUProfilerPanel();
    ~GPUProfilerPanel() override;

    std::string PanelName() const override { return "GPU Profiler"; }
    std::string PanelIcon() const override { return "[G]"; }

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float dt) override;
    void OnDraw(const UI::UIStyle& style) override;

    void SetProfiler(GPUProfiler* profiler);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Tools
