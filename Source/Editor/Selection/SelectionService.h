#pragma once

#include <cstdint>
#include <string>

namespace nf
{
    enum class SelectionKind
    {
        None = 0,
        Asset,
        WorldObject,
        Chunk,
        Voxel
    };

    struct SelectionHandle
    {
        SelectionKind kind = SelectionKind::None;
        std::uint64_t id = 0;
        std::string label;
    };

    class SelectionService
    {
    public:
        void Clear();
        void Select(const SelectionHandle& handle);

        bool HasSelection() const noexcept;
        const SelectionHandle& GetSelection() const noexcept;
        SelectionKind GetSelectionKind() const noexcept;
        std::string GetSelectionLabel() const;

        bool IsSelectionChanged() const noexcept;
        void ClearSelectionChanged() noexcept;

    private:
        SelectionHandle m_current;
        bool m_selectionChanged = false;
    };
}
