#ifndef DIALOGUEBOX_H
#define DIALOGUEBOX_H

#include <string>

class Renderer;

class DialogueBox {
public:
    DialogueBox() = default;
    ~DialogueBox() = default;

    void Show(const std::string& text);
    void Hide();
    void Render(Renderer* renderer);

    bool IsVisible() const { return m_visible; }

private:
    std::string m_text;
    bool m_visible = false;
};

#endif // DIALOGUEBOX_H
