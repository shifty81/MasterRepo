#include "Engine/Input/InputManager.h"
#include <iostream>

int main() {
    Engine::Input::InputManager input;
    input.Init();
    input.BindAction(Engine::Input::InputAction::Jump, Engine::Input::InputDevice::Keyboard, 32, "Space");
    if (input.HasBinding(Engine::Input::InputAction::Jump)) {
        std::cout << "Jump action bound to Space key.\n";
    }
    input.Shutdown();
    return 0;
}