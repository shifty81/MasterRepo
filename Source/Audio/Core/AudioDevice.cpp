#include "Audio/Core/AudioDevice.h"
#include <algorithm>

namespace NF {

bool AudioDevice::Init() {
    m_Initialised = true;
    return true;
}

void AudioDevice::Shutdown() {
    m_Initialised = false;
}

void AudioDevice::SetMasterVolume(float volume) noexcept {
    m_MasterVolume = std::clamp(volume, 0.f, 1.f);
}

float AudioDevice::GetMasterVolume() const noexcept {
    return m_MasterVolume;
}

} // namespace NF
