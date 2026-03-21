#include "Engine/Audio/AudioEngine.h"
#include <iostream>

int main() {
    Engine::Audio::AudioEngine audio;
    audio.Init();
    auto soundId = audio.LoadSound("test_sound.wav");
    if (audio.HasSound(soundId)) {
        audio.Play(soundId);
        std::cout << "Playing sound with ID: " << soundId << std::endl;
    } else {
        std::cout << "Failed to load sound (stub or missing file)." << std::endl;
    }
    audio.Shutdown();
    return 0;
}
