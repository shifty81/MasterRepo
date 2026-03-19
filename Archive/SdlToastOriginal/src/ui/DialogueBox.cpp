#include "DialogueBox.h"
#include "../engine/Renderer.h"

void DialogueBox::Show(const std::string& text) {
    m_text = text;
    m_visible = true;
}

void DialogueBox::Hide() {
    m_visible = false;
    m_text.clear();
}

void DialogueBox::Render(Renderer* renderer) {
    if (!m_visible) return;
    
    // Draw dialogue box background
    renderer->FillRect(50, 400, 700, 150, 0, 0, 0, 200);
    renderer->DrawRect(50, 400, 700, 150, 255, 255, 255);
    
    // Render dialogue text using Raylib's built-in text rendering
    if (!m_text.empty()) {
        renderer->DrawGameText(m_text.c_str(), 70, 420, 18, 255, 255, 255);
    }
}
