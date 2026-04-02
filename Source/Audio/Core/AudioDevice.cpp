#include "Audio/Core/AudioDevice.h"
#include "Core/Logging/Log.h"
#include <algorithm>
#include <string>

namespace NF {

bool AudioDevice::Init(AudioBackend backend) {
    if (m_Initialised) return true;

    m_Backend = backend;

#ifdef _WIN32
    if (backend == AudioBackend::WASAPI) {
        // TODO: Phase 9+ — real WASAPI device enumeration + exclusive mode.
        Logger::Log(LogLevel::Info, "Audio",
                    "WASAPI requested — falling back to Null backend on this build");
        m_Backend = AudioBackend::Null;
    }
#else
    if (backend == AudioBackend::WASAPI) {
        Logger::Log(LogLevel::Info, "Audio",
                    "WASAPI unavailable on this platform — using Null backend");
        m_Backend = AudioBackend::Null;
    }
#endif

    m_SampleRate = 44100;
    m_Channels   = 2;
    m_Initialised = true;

    Logger::Log(LogLevel::Info, "Audio",
                "AudioDevice initialised — backend=" +
                std::string(m_Backend == AudioBackend::Null ? "Null" : "WASAPI") +
                " rate=" + std::to_string(m_SampleRate) +
                " ch=" + std::to_string(m_Channels));
    return true;
}

void AudioDevice::Shutdown() {
    if (!m_Initialised) return;
    m_Initialised = false;
    Logger::Log(LogLevel::Info, "Audio", "AudioDevice shutdown");
}

void AudioDevice::SetMasterVolume(float volume) noexcept {
    m_MasterVolume = std::clamp(volume, 0.f, 1.f);
}

float AudioDevice::GetMasterVolume() const noexcept {
    return m_MasterVolume;
}

} // namespace NF
