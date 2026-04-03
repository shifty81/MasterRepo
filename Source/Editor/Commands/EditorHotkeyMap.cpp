#include "Editor/Commands/EditorHotkeyMap.h"

namespace nf
{
    std::string HotkeyChord::ToString() const
    {
        std::string value;
        if (ctrl) value += "Ctrl+";
        if (shift) value += "Shift+";
        if (alt) value += "Alt+";
        value += key;
        return value;
    }

    bool HotkeyChord::operator==(const HotkeyChord& other) const noexcept
    {
        return ctrl == other.ctrl
            && shift == other.shift
            && alt == other.alt
            && key == other.key;
    }

    std::size_t HotkeyChordHasher::operator()(const HotkeyChord& chord) const noexcept
    {
        std::size_t h = std::hash<std::string>{}(chord.key);
        h ^= std::hash<bool>{}(chord.ctrl) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<bool>{}(chord.shift) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<bool>{}(chord.alt) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }

    void EditorHotkeyMap::Bind(const HotkeyChord& chord, const std::string& commandId)
    {
        if (chord.key.empty() || commandId.empty())
        {
            return;
        }

        m_bindings[chord] = commandId;
    }

    const std::string* EditorHotkeyMap::Find(const HotkeyChord& chord) const
    {
        const auto it = m_bindings.find(chord);
        return it != m_bindings.end() ? &it->second : nullptr;
    }

    void EditorHotkeyMap::BuildDefaultBindings()
    {
        m_bindings.clear();

        Bind({true, false, false, "S"}, "World.SaveDevWorld");
        Bind({true, true, false, "S"}, "World.SaveDevWorldAs");
        Bind({true, false, false, "R"}, "World.ReloadDevWorld");
        Bind({false, false, false, "Q"}, "Tools.SelectMode");
        Bind({false, false, false, "1"}, "Tools.VoxelInspectMode");
        Bind({false, false, false, "2"}, "Tools.VoxelAddMode");
        Bind({false, false, false, "3"}, "Tools.VoxelRemoveMode");
        Bind({false, false, false, "`"}, "View.ToggleConsole");
    }
}
