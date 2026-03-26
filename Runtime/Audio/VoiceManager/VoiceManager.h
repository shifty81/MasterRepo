#pragma once
/**
 * @file VoiceManager.h
 * @brief Manages voiced dialogue lines: queuing, priority, subtitles, lip-sync.
 *
 * The VoiceManager mediates between the ConversationGraph (or any other
 * game system) and the audio backend.  Features:
 *   - Per-actor voice channel (only one line plays at a time per speaker)
 *   - Priority queue (combat callouts > ambient barks > ambient idle)
 *   - Auto-subtitle generation (display text while line plays)
 *   - Lip-sync data (.phoneme file) association
 *   - Voice-over distance falloff (world-space speaker position)
 *   - Cooldown per line to avoid repetition
 *
 * Typical usage:
 * @code
 *   VoiceManager vm;
 *   vm.Init();
 *   VoiceLine line;
 *   line.speakerId = "npc_blacksmith";
 *   line.audioPath = "VO/blacksmith_greet_01.ogg";
 *   line.text      = "Welcome, traveller!";
 *   line.priority  = 10;
 *   vm.PlayLine(line);
 *   vm.Update(dt);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

enum class VoicePriority : uint8_t {
    Ambient=0, Bark=5, Dialogue=10, Combat=20, Cutscene=30
};

struct VoiceLine {
    std::string   speakerId;
    std::string   audioPath;
    std::string   text;              ///< subtitle text
    std::string   locKey;
    std::string   phonemePath;       ///< optional lip-sync data
    VoicePriority priority{VoicePriority::Dialogue};
    float         speakerPos[3]{};   ///< world position for 3D audio
    float         cooldown{5.f};     ///< seconds before this line can repeat
    bool          interruptible{true};
};

struct VoiceLineState {
    VoiceLine line;
    float     elapsed{0.f};
    float     duration{0.f};
    bool      playing{false};
};

class VoiceManager {
public:
    VoiceManager();
    ~VoiceManager();

    void Init();
    void Shutdown();

    // Playback
    void     PlayLine(const VoiceLine& line);
    void     StopSpeaker(const std::string& speakerId);
    void     StopAll();
    bool     IsSpeakerBusy(const std::string& speakerId) const;
    VoiceLineState GetSpeakerState(const std::string& speakerId) const;

    // Subtitle
    std::string GetCurrentSubtitle(const std::string& speakerId) const;
    std::vector<std::pair<std::string,std::string>> AllActiveSubtitles() const;

    // Cooldowns
    bool IsOnCooldown(const std::string& speakerId, const std::string& audioPath) const;
    void SetCooldownOverride(const std::string& audioPath, float cooldown);

    // Position update (for 3D audio falloff)
    void SetSpeakerPosition(const std::string& speakerId, const float pos[3]);

    void Update(float dt);

    // Callbacks
    void OnLineStarted(std::function<void(const VoiceLine&)> cb);
    void OnLineFinished(std::function<void(const VoiceLine&)> cb);
    void OnSubtitleChanged(std::function<void(const std::string& speakerId,
                                               const std::string& text)> cb);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Runtime
