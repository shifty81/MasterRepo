#pragma once
/**
 * @file SpeechSystem.h
 * @brief Text-to-phoneme + lipsync + voice line playback: schedule, viseme, pitch.
 *
 * Features:
 *   - RegisterVoice(voiceId, name): add a named voice profile
 *   - SetVoicePitch(voiceId, semitones) / SetVoiceRate(voiceId, rate)
 *   - Speak(voiceId, text) → lineId: enqueue a speech line
 *   - Stop(voiceId) / StopAll()
 *   - Tick(dt): advance playback, emit viseme callbacks
 *   - GetCurrentViseme(voiceId) → uint8_t [0=silence..15]
 *   - GetPlaybackProgress(voiceId) → float [0,1]
 *   - IsPlaying(voiceId) → bool
 *   - SetOnViseme(voiceId, cb): callback(visemeId, blendWeight)
 *   - SetOnLineStart(voiceId, cb): callback(lineId, text)
 *   - SetOnLineEnd(voiceId, cb): callback(lineId)
 *   - SetPhonemeMap(map): custom character→phoneme override
 *   - Preprocess(text) → string: expand abbreviations, numbers
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Engine {

class SpeechSystem {
public:
    SpeechSystem();
    ~SpeechSystem();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Voice management
    void RegisterVoice  (uint32_t voiceId, const std::string& name);
    void SetVoicePitch  (uint32_t voiceId, float semitones);
    void SetVoiceRate   (uint32_t voiceId, float rate);

    // Playback
    uint32_t Speak  (uint32_t voiceId, const std::string& text);
    void     Stop   (uint32_t voiceId);
    void     StopAll();

    // Per-frame
    void Tick(float dt);

    // Query
    uint8_t GetCurrentViseme     (uint32_t voiceId) const;
    float   GetPlaybackProgress  (uint32_t voiceId) const;
    bool    IsPlaying            (uint32_t voiceId) const;

    // Callbacks
    void SetOnViseme   (uint32_t voiceId,
                        std::function<void(uint8_t visemeId, float weight)> cb);
    void SetOnLineStart(uint32_t voiceId,
                        std::function<void(uint32_t lineId, const std::string&)> cb);
    void SetOnLineEnd  (uint32_t voiceId,
                        std::function<void(uint32_t lineId)> cb);

    // Text processing
    void        SetPhonemeMap(const std::unordered_map<char,std::string>& map);
    std::string Preprocess   (const std::string& text) const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
