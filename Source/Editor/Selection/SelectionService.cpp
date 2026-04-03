#include "Editor/Selection/SelectionService.h"

namespace nf
{
    void SelectionService::Clear()
    {
        m_current = {};
        m_selectionChanged = true;
    }

    void SelectionService::Select(const SelectionHandle& handle)
    {
        m_current = handle;
        m_selectionChanged = true;
    }

    bool SelectionService::HasSelection() const noexcept
    {
        return m_current.kind != SelectionKind::None;
    }

    const SelectionHandle& SelectionService::GetSelection() const noexcept
    {
        return m_current;
    }

    SelectionKind SelectionService::GetSelectionKind() const noexcept
    {
        return m_current.kind;
    }

    std::string SelectionService::GetSelectionLabel() const
    {
        if (!HasSelection())
        {
            return "No Selection";
        }

        return m_current.label.empty() ? "Unnamed Selection" : m_current.label;
    }

    bool SelectionService::IsSelectionChanged() const noexcept
    {
        return m_selectionChanged;
    }

    void SelectionService::ClearSelectionChanged() noexcept
    {
        m_selectionChanged = false;
    }
}
