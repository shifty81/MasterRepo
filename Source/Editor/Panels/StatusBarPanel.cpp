#include "Editor/Panels/StatusBarPanel.h"

#include <sstream>

namespace nf
{
    void StatusBarPanel::SetState(StatusBarState state)
    {
        m_state = std::move(state);
    }

    const StatusBarState& StatusBarPanel::GetState() const noexcept
    {
        return m_state;
    }

    std::string StatusBarPanel::BuildDisplayString() const
    {
        std::ostringstream oss;
        oss << "World: " << m_state.activeWorld
            << " | Tool: " << m_state.activeTool
            << " | Dirty: " << (m_state.dirty ? "Yes" : "No")
            << " | Selection: " << m_state.selectionSummary
            << " | Ready: " << m_state.worldReadiness
            << " | Chunks: " << m_state.loadedChunkCount;
        return oss.str();
    }
}
