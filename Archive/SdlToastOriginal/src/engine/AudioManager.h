#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include <raylib.h>
#include <string>
#include <unordered_map>

class AudioManager {
public:
    AudioManager() = default;
    ~AudioManager();

    bool Initialize();
    
    void PlayMusicFile(const std::string& filepath, int loops = -1);
    void PlaySoundFile(const std::string& filepath);
    void StopMusicPlayback();
    
    void SetMusicVolume(int volume); // 0-128
    void SetSoundVolume(int volume); // 0-128

private:
    Music m_currentMusic;
    bool m_musicLoaded;
    std::unordered_map<std::string, Sound> m_sounds;
};

#endif // AUDIOMANAGER_H
