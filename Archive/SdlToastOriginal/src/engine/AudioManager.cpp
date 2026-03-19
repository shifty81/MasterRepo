#include "AudioManager.h"
#include "Logger.h"
#include <iostream>

AudioManager::~AudioManager() {
    for (auto& pair : m_sounds) {
        UnloadSound(pair.second);
    }
    m_sounds.clear();
    
    if (m_musicLoaded) {
        UnloadMusicStream(m_currentMusic);
    }
    
    CloseAudioDevice();
}

bool AudioManager::Initialize() {
    InitAudioDevice();
    
    if (!IsAudioDeviceReady()) {
        Logger::Instance().Error("Audio device initialization failed");
        return false;
    }
    
    m_musicLoaded = false;
    return true;
}

void AudioManager::PlayMusicFile(const std::string& filepath, int loops) {
    if (m_musicLoaded) {
        UnloadMusicStream(m_currentMusic);
    }
    
    m_currentMusic = LoadMusicStream(filepath.c_str());
    if (m_currentMusic.frameCount == 0) {
        Logger::Instance().Error("Failed to load music " + filepath);
        m_musicLoaded = false;
        return;
    }
    
    m_musicLoaded = true;
    m_currentMusic.looping = (loops != 0);
    PlayMusicStream(m_currentMusic);
}

void AudioManager::PlaySoundFile(const std::string& filepath) {
    Sound sound = {};
    
    auto it = m_sounds.find(filepath);
    if (it != m_sounds.end()) {
        sound = it->second;
    } else {
        sound = LoadSound(filepath.c_str());
        if (sound.frameCount == 0) {
            Logger::Instance().Error("Failed to load sound " + filepath);
            return;
        }
        m_sounds[filepath] = sound;
    }
    
    PlaySound(sound);
}

void AudioManager::StopMusicPlayback() {
    if (m_musicLoaded) {
        StopMusicStream(m_currentMusic);
    }
}

void AudioManager::SetMusicVolume(int volume) {
    if (m_musicLoaded) {
        ::SetMusicVolume(m_currentMusic, volume / 128.0f);
    }
}

void AudioManager::SetSoundVolume(int volume) {
    SetMasterVolume(volume / 128.0f);
}
