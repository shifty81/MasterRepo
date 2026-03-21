#pragma once
// AudioPipeline — Engine-level procedural audio pipeline
//
// Bridges Engine::Audio::AudioEngine with PCG::AudioGenerator and
// optional third-party backends (SuperCollider OSC, FluidSynth MIDI).
//
// Responsibilities:
//   • Procedural sound-effect synthesis (via PCG::AudioGenerator)
//   • Adaptive music layer (layered stems with dynamic mix)
//   • SuperCollider OSC stub — sends /play_synth messages
//   • FluidSynth stub — MIDI note-on / soundfont playback
//   • Sound event queue consumed by AudioEngine each frame
//
// All heavy synthesis is run off the main thread via a worker queue.

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace Engine::Audio {

// ─────────────────────────────────────────────────────────────────────────────
// Sound event — produced by game code, consumed by AudioPipeline
// ─────────────────────────────────────────────────────────────────────────────

enum class SoundEventType {
    PlaySFX,        // procedural SFX
    PlayMusic,      // layered music stem
    PlayMIDI,       // FluidSynth MIDI note
    PlaySC,         // SuperCollider OSC synth
    StopAll,
    SetMusicIntensity,  // 0..1 — controls dynamic mix layers
    FadeOut,
};

struct SoundEvent {
    SoundEventType type         = SoundEventType::PlaySFX;
    std::string    id;           // sfx key / music stem ID / synth name
    float          volume       = 1.0f;
    float          pitch        = 1.0f;
    float          pan          = 0.0f;   // -1 (L) … +1 (R)
    float          fadeTimeSec  = 0.0f;
    int            midiNote     = 60;     // for PlayMIDI
    int            midiVelocity = 100;
    float          intensity    = 1.0f;   // for SetMusicIntensity
    std::array<float,3> position = {};    // 3-D source for spatial audio
};

// ─────────────────────────────────────────────────────────────────────────────
// Adaptive music stem
// ─────────────────────────────────────────────────────────────────────────────

struct MusicStem {
    std::string id;
    std::string assetPath;       // .wav / .ogg
    float       baseVolume = 1.0f;
    float       intensityThresholdLow  = 0.0f; // fades in above this
    float       intensityThresholdHigh = 1.0f; // muted above this
    bool        looping    = true;
};

struct AdaptiveMusicConfig {
    std::vector<MusicStem> stems;
    float       bpm        = 120.f;
    int         beatsPerBar = 4;
    bool        syncToBar  = true;   // queue stem changes to bar boundaries
};

// ─────────────────────────────────────────────────────────────────────────────
// SuperCollider stub
// ─────────────────────────────────────────────────────────────────────────────

struct SCConfig {
    std::string  host    = "127.0.0.1";
    uint16_t     port    = 57110;
    bool         enabled = false;
};

// ─────────────────────────────────────────────────────────────────────────────
// FluidSynth stub
// ─────────────────────────────────────────────────────────────────────────────

struct FluidSynthConfig {
    std::string soundfontPath;
    int         audioDriver = 0;  // 0 = null driver (stub)
    bool        enabled = false;
};

// ─────────────────────────────────────────────────────────────────────────────
// AudioPipelineConfig
// ─────────────────────────────────────────────────────────────────────────────

struct AudioPipelineConfig {
    AdaptiveMusicConfig music;
    SCConfig            sc;
    FluidSynthConfig    fluid;
    int                 workerThreads  = 1;
    int                 eventQueueSize = 256;
    bool                spatialAudio   = true;
    float               masterVolume   = 1.0f;
};

// ─────────────────────────────────────────────────────────────────────────────
// AudioPipeline
// ─────────────────────────────────────────────────────────────────────────────

class AudioPipeline {
public:
    explicit AudioPipeline(AudioPipelineConfig cfg = {});
    ~AudioPipeline();

    // ── Lifecycle ────────────────────────────────────────────────────────────

    bool Init();
    void Shutdown();

    // ── Per-frame ────────────────────────────────────────────────────────────

    /// Drain the event queue and dispatch to backends. Call once per frame.
    void Update(float dt);

    // ── Event submission (thread-safe) ───────────────────────────────────────

    void Post(const SoundEvent& ev);
    void PlaySFX(const std::string& id, float vol = 1.f, float pitch = 1.f);
    void PlayMusic(const std::string& stemId, float intensity = 1.f);
    void SetMusicIntensity(float intensity);   // 0..1
    void StopAll();

    // ── Procedural SFX generation ────────────────────────────────────────────

    /// Generate and register a procedural SFX in the internal cache.
    bool GenerateProcSFX(const std::string& id, const std::string& preset);

    // ── SuperCollider ────────────────────────────────────────────────────────

    /// Send a raw OSC message to SuperCollider (stub — logs if not connected).
    bool SendOSC(const std::string& address,
                 const std::vector<float>& args = {});

    // ── FluidSynth ───────────────────────────────────────────────────────────

    void FluidNoteOn(int channel, int note, int velocity);
    void FluidNoteOff(int channel, int note);
    void FluidProgramChange(int channel, int program);

    // ── Adaptive music ───────────────────────────────────────────────────────

    void RegisterStem(const MusicStem& stem);
    void UnregisterStem(const std::string& id);

    // ── Stats ────────────────────────────────────────────────────────────────

    int  EventsPosted()     const;
    int  EventsProcessed()  const;
    int  ActiveStemCount()  const;

    const AudioPipelineConfig& GetConfig() const;
    void SetMasterVolume(float v);

    // ── Callbacks ────────────────────────────────────────────────────────────

    using EventFn = std::function<void(const SoundEvent&)>;
    void SetEventCallback(EventFn fn);   // called for every dispatched event

private:
    AudioPipelineConfig      m_cfg;
    bool                     m_initialised = false;
    float                    m_musicIntensity = 0.f;

    std::vector<SoundEvent>  m_queue;
    std::vector<MusicStem>   m_stems;

    int   m_eventsPosted    = 0;
    int   m_eventsProcessed = 0;

    EventFn m_eventCb;

    void DispatchEvent(const SoundEvent& ev);
    void UpdateAdaptiveMusic(float dt);
};

} // namespace Engine::Audio
