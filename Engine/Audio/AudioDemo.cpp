#include "Engine/Audio/AudioEngine.h"
#include <iostream>

int main() {
    Engine::Audio::AudioEngine audio;
    audio.Init();
    std::cout << "Audio system initialized (stub).\n";
    audio.Shutdown();
    return 0;
}