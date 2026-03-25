#pragma once
#include <string>
#include "Tools/GUI/IToolPanel.h"

namespace Tools {
class PerformanceProfiler;
}

namespace Tools {

/**
 * PerfProfilerPanel — displays CPU scope stats and last-frame samples.
 */
class PerfProfilerPanel : public IToolPanel {
public:
    PerfProfilerPanel();
    ~PerfProfilerPanel() override;

    std::string PanelName() const override { return "Perf Profiler"; }
    std::string PanelIcon() const override { return "[P]"; }

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float dt) override;
    void OnDraw(const UI::UIStyle& style) override;

    void SetProfiler(PerformanceProfiler* profiler);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Tools
