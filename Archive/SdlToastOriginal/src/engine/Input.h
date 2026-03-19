#ifndef INPUT_H
#define INPUT_H

#include <raylib.h>

/**
 * Input handling system for keyboard and gamepad
 */
class Input {
public:
    Input();
    ~Input() = default;

    void Update();

    // Keyboard
    bool IsKeyDown(int key) const;
    bool IsKeyPressed(int key) const;
    bool IsKeyReleased(int key) const;

    // Mouse
    void GetMousePosition(int& x, int& y) const;
    bool IsMouseButtonDown(int button) const;
};

#endif // INPUT_H
