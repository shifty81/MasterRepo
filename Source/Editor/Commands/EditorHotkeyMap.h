#pragma once

#include <string>
#include <unordered_map>

namespace nf
{
    struct HotkeyChord
    {
        bool ctrl = false;
        bool shift = false;
        bool alt = false;
        std::string key;

        [[nodiscard]] std::string ToString() const;
        bool operator==(const HotkeyChord& other) const noexcept;
    };

    struct HotkeyChordHasher
    {
        std::size_t operator()(const HotkeyChord& chord) const noexcept;
    };

    class EditorHotkeyMap
    {
    public:
        void Bind(const HotkeyChord& chord, const std::string& commandId);
        const std::string* Find(const HotkeyChord& chord) const;
        void BuildDefaultBindings();

    private:
        std::unordered_map<HotkeyChord, std::string, HotkeyChordHasher> m_bindings;
    };
}
