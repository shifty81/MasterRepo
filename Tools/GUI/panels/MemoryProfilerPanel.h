#pragma once
#include <string>
#include <vector>
#include "Tools/GUI/IToolPanel.h"

namespace Tools {
class MemoryProfiler;
}

namespace Tools {

/**
 * MemoryProfilerPanel — per-tag memory table, snapshots, and diff view.
 */
class MemoryProfilerPanel : public IToolPanel {
public:
    MemoryProfilerPanel();
    ~MemoryProfilerPanel() override;

    std::string PanelName() const override { return "Memory Profiler"; }
    std::string PanelIcon() const override { return "[M]"; }

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float dt) override;
    void OnDraw(const UI::UIStyle& style) override;

    void SetProfiler(MemoryProfiler* profiler);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Tools
