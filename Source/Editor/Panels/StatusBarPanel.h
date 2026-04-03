#pragma once

#include <string>

namespace nf
{
    struct StatusBarState
    {
        std::string activeWorld = "No World";
        std::string activeTool = "Select";
        std::string selectionSummary = "No Selection";
        std::string worldReadiness = "Not Ready";
        bool dirty = false;
        int loadedChunkCount = 0;
    };

    class StatusBarPanel
    {
    public:
        void SetState(StatusBarState state);
        const StatusBarState& GetState() const noexcept;

        std::string BuildDisplayString() const;

    private:
        StatusBarState m_state;
    };
}
