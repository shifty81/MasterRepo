#include "Engine/Audio/AudioPipeline.h"
#include <algorithm>
#include <cmath>

namespace Engine::Audio {

AudioPipeline::AudioPipeline(AudioPipelineConfig cfg)
    : m_cfg(std::move(cfg))
{}

AudioPipeline::~AudioPipeline() { Shutdown(); }

// ── Lifecycle ─────────────────────────────────────────────────────────────────

bool AudioPipeline::Init()
{
    if (m_initialised) return true;
    // Register stems from config
    for (const auto& stem : m_cfg.music.stems)
        m_stems.push_back(stem);
    m_initialised = true;
    return true;
}

void AudioPipeline::Shutdown()
{
    if (!m_initialised) return;
    m_queue.clear();
    m_stems.clear();
    m_initialised = false;
}

// ── Per-frame ────────────────────────────────────────────────────────────────

void AudioPipeline::Update(float dt)
{
    UpdateAdaptiveMusic(dt);

    // Drain queue
    std::vector<SoundEvent> current;
    std::swap(current, m_queue);

    for (auto& ev : current) {
        DispatchEvent(ev);
        ++m_eventsProcessed;
    }
}

// ── Event submission ─────────────────────────────────────────────────────────

void AudioPipeline::Post(const SoundEvent& ev)
{
    if ((int)m_queue.size() < m_cfg.eventQueueSize) {
        m_queue.push_back(ev);
        ++m_eventsPosted;
    }
}

void AudioPipeline::PlaySFX(const std::string& id, float vol, float pitch)
{
    SoundEvent ev;
    ev.type   = SoundEventType::PlaySFX;
    ev.id     = id;
    ev.volume = vol;
    ev.pitch  = pitch;
    Post(ev);
}

void AudioPipeline::PlayMusic(const std::string& stemId, float intensity)
{
    SoundEvent ev;
    ev.type      = SoundEventType::PlayMusic;
    ev.id        = stemId;
    ev.intensity = intensity;
    Post(ev);
}

void AudioPipeline::SetMusicIntensity(float intensity)
{
    m_musicIntensity = std::max(0.f, std::min(1.f, intensity));
    SoundEvent ev;
    ev.type      = SoundEventType::SetMusicIntensity;
    ev.intensity = m_musicIntensity;
    Post(ev);
}

void AudioPipeline::StopAll()
{
    SoundEvent ev;
    ev.type = SoundEventType::StopAll;
    Post(ev);
}

// ── Procedural SFX ───────────────────────────────────────────────────────────

bool AudioPipeline::GenerateProcSFX(const std::string& /*id*/,
                                      const std::string& /*preset*/)
{
    // Delegate to PCG::AudioGenerator in a real implementation.
    // Stub: always succeeds.
    return true;
}

// ── SuperCollider ────────────────────────────────────────────────────────────

bool AudioPipeline::SendOSC(const std::string& /*address*/,
                              const std::vector<float>& /*args*/)
{
    if (!m_cfg.sc.enabled) return false;
    // OSC socket send stub (requires sc server at cfg.sc.host:port)
    return true;
}

// ── FluidSynth ───────────────────────────────────────────────────────────────

void AudioPipeline::FluidNoteOn(int /*channel*/, int /*note*/,
                                  int /*velocity*/)
{
    if (!m_cfg.fluid.enabled) return;
    // FluidSynth API stub
}

void AudioPipeline::FluidNoteOff(int /*channel*/, int /*note*/)
{
    if (!m_cfg.fluid.enabled) return;
}

void AudioPipeline::FluidProgramChange(int /*channel*/, int /*program*/)
{
    if (!m_cfg.fluid.enabled) return;
}

// ── Adaptive music ────────────────────────────────────────────────────────────

void AudioPipeline::RegisterStem(const MusicStem& stem)
{
    auto it = std::find_if(m_stems.begin(), m_stems.end(),
                           [&](const MusicStem& s){ return s.id == stem.id; });
    if (it != m_stems.end()) *it = stem;
    else                     m_stems.push_back(stem);
}

void AudioPipeline::UnregisterStem(const std::string& id)
{
    m_stems.erase(std::remove_if(m_stems.begin(), m_stems.end(),
                                 [&](const MusicStem& s){ return s.id == id; }),
                  m_stems.end());
}

// ── Stats ─────────────────────────────────────────────────────────────────────

int  AudioPipeline::EventsPosted()    const { return m_eventsPosted; }
int  AudioPipeline::EventsProcessed() const { return m_eventsProcessed; }
int  AudioPipeline::ActiveStemCount() const { return (int)m_stems.size(); }

const AudioPipelineConfig& AudioPipeline::GetConfig() const { return m_cfg; }

void AudioPipeline::SetMasterVolume(float v)
{
    m_cfg.masterVolume = std::max(0.f, std::min(1.f, v));
}

void AudioPipeline::SetEventCallback(EventFn fn) { m_eventCb = std::move(fn); }

// ── Private ───────────────────────────────────────────────────────────────────

void AudioPipeline::DispatchEvent(const SoundEvent& ev)
{
    if (m_eventCb) m_eventCb(ev);

    switch (ev.type) {
        case SoundEventType::PlaySFX:
            // Hand off to Engine::Audio::AudioEngine (stub)
            break;
        case SoundEventType::PlayMusic:
            break;
        case SoundEventType::PlayMIDI:
            FluidNoteOn(0, ev.midiNote, ev.midiVelocity);
            break;
        case SoundEventType::PlaySC:
            SendOSC("/play_synth", {ev.volume, ev.pitch});
            break;
        case SoundEventType::StopAll:
            break;
        case SoundEventType::SetMusicIntensity:
            m_musicIntensity = ev.intensity;
            break;
        case SoundEventType::FadeOut:
            break;
    }
}

void AudioPipeline::UpdateAdaptiveMusic(float /*dt*/)
{
    // Adjust stem volumes based on current intensity
    // In a real engine: ramp volume of each stem AudioEngine channel
    for (const auto& stem : m_stems) {
        float vol = stem.baseVolume;
        if (m_musicIntensity < stem.intensityThresholdLow)
            vol = 0.f;
        else if (m_musicIntensity > stem.intensityThresholdHigh)
            vol = 0.f;
        (void)vol; // passed to AudioEngine in real implementation
    }
}

} // namespace Engine::Audio
