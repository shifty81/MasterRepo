#include "Menu.h"
#include "../engine/Renderer.h"

void Menu::AddOption(const std::string& text) {
    m_options.push_back(text);
}

void Menu::Render(Renderer* renderer) {
    if (m_options.empty()) return;

    int startY = 200;
    int x = 300;
    for (size_t i = 0; i < m_options.size(); ++i) {
        unsigned char r = 200, g = 200, b = 200;
        std::string line;
        if (static_cast<int>(i) == m_selectedIndex) {
            r = 255; g = 255; b = 50;
            line = "> " + m_options[i];
        } else {
            line = "  " + m_options[i];
        }
        renderer->DrawGameText(line.c_str(), x, startY + static_cast<int>(i) * 30, 20, r, g, b);
    }
}

void Menu::SelectNext() {
    if (!m_options.empty()) {
        m_selectedIndex = (m_selectedIndex + 1) % m_options.size();
    }
}

void Menu::SelectPrevious() {
    if (!m_options.empty()) {
        m_selectedIndex = (m_selectedIndex - 1 + m_options.size()) % m_options.size();
    }
}
