#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "Audio/Core/AudioDevice.h"
#include "Audio/Core/SoundAsset.h"
#include "Audio/Core/SoundBank.h"
#include "Audio/Mixer/AudioMixer.h"
#include "Audio/Spatial/SpatialAudio.h"
#include "Game/App/Orchestrator.h"

#include <cmath>
#include <fstream>
#include <filesystem>
#include <cstring>

using namespace NF;
using Catch::Approx;

// ============================================================================
// AudioDevice
// ============================================================================

TEST_CASE("AudioDevice: default state before Init", "[Audio]") {
    AudioDevice device;
    REQUIRE_FALSE(device.IsInitialized());
    REQUIRE(device.GetMasterVolume() == Approx(1.f));
    REQUIRE(device.GetBackend() == AudioBackend::Null);
}

TEST_CASE("AudioDevice: Init and Shutdown lifecycle", "[Audio]") {
    AudioDevice device;
    REQUIRE(device.Init(AudioBackend::Null));
    REQUIRE(device.IsInitialized());
    REQUIRE(device.GetBackend() == AudioBackend::Null);
    REQUIRE(device.GetSampleRate() == 44100);
    REQUIRE(device.GetChannels() == 2);

    device.Shutdown();
    REQUIRE_FALSE(device.IsInitialized());
}

TEST_CASE("AudioDevice: master volume clamps to [0,1]", "[Audio]") {
    AudioDevice device;
    device.Init();

    device.SetMasterVolume(0.5f);
    REQUIRE(device.GetMasterVolume() == Approx(0.5f));

    device.SetMasterVolume(-1.f);
    REQUIRE(device.GetMasterVolume() == Approx(0.f));

    device.SetMasterVolume(2.f);
    REQUIRE(device.GetMasterVolume() == Approx(1.f));

    device.Shutdown();
}

TEST_CASE("AudioDevice: WASAPI backend falls back to Null on Linux", "[Audio]") {
    AudioDevice device;
    REQUIRE(device.Init(AudioBackend::WASAPI));
    // On CI (Linux) this should fall back to Null.
    REQUIRE(device.GetBackend() == AudioBackend::Null);
    device.Shutdown();
}

// ============================================================================
// SoundAsset
// ============================================================================

TEST_CASE("SoundAsset: default is invalid", "[Audio]") {
    SoundAsset asset;
    REQUIRE_FALSE(asset.IsValid());
    REQUIRE(asset.FrameCount() == 0);
}

TEST_CASE("SoundAsset: populated asset reports valid", "[Audio]") {
    SoundAsset asset;
    asset.SampleRate = 44100;
    asset.Channels   = 2;
    asset.Samples    = {0.f, 0.f, 1.f, -1.f};
    asset.Duration   = 0.01f;

    REQUIRE(asset.IsValid());
    REQUIRE(asset.FrameCount() == 2);   // 4 samples / 2 channels
}

// ============================================================================
// SoundBank — procedural generation
// ============================================================================

TEST_CASE("SoundBank: GenerateTone creates a valid asset", "[Audio]") {
    SoundBank bank;
    SoundId id = bank.GenerateTone("test_tone", 440.f, 0.5f, 0.8f);

    REQUIRE(id != 0);
    REQUIRE(bank.Count() == 1);

    const SoundAsset* asset = bank.Get(id);
    REQUIRE(asset != nullptr);
    REQUIRE(asset->Name == "test_tone");
    REQUIRE(asset->SampleRate == 44100);
    REQUIRE(asset->Channels == 1);
    REQUIRE(asset->Duration == Approx(0.5f));
    REQUIRE(asset->IsValid());
    REQUIRE(asset->FrameCount() == 22050);   // 0.5s * 44100
}

TEST_CASE("SoundBank: GenerateNoise creates a valid asset", "[Audio]") {
    SoundBank bank;
    SoundId id = bank.GenerateNoise("test_noise", 1.0f, 0.3f);

    REQUIRE(id != 0);
    const SoundAsset* asset = bank.Get(id);
    REQUIRE(asset != nullptr);
    REQUIRE(asset->Channels == 1);
    REQUIRE(asset->Duration == Approx(1.0f));
    REQUIRE(asset->FrameCount() == 44100);

    // Verify noise samples are within [-0.3, 0.3].
    for (float s : asset->Samples) {
        REQUIRE(s >= -0.3f);
        REQUIRE(s <= 0.3f);
    }
}

TEST_CASE("SoundBank: GetByName lookup", "[Audio]") {
    SoundBank bank;
    SoundId id = bank.GenerateTone("lookup_test", 440.f, 0.1f);

    const SoundAsset* byName = bank.GetByName("lookup_test");
    REQUIRE(byName != nullptr);
    REQUIRE(byName->Id == id);

    REQUIRE(bank.GetByName("nonexistent") == nullptr);
}

TEST_CASE("SoundBank: Clear removes all assets", "[Audio]") {
    SoundBank bank;
    bank.GenerateTone("a", 440.f, 0.1f);
    bank.GenerateNoise("b", 0.1f);
    REQUIRE(bank.Count() == 2);

    bank.Clear();
    REQUIRE(bank.Count() == 0);
}

// ============================================================================
// SoundBank — WAV loading
// ============================================================================

namespace {

/// @brief Write a minimal valid 16-bit PCM mono WAV to disk.
void WriteTestWav(const std::string& path, uint32_t sampleRate,
                   uint16_t channels, const std::vector<int16_t>& samples)
{
    const uint32_t dataSize  = static_cast<uint32_t>(samples.size() * 2);
    const uint32_t riffSize  = 36 + dataSize;
    const uint16_t bitsPerSample = 16;
    const uint16_t blockAlign = channels * (bitsPerSample / 8);
    const uint32_t byteRate  = sampleRate * blockAlign;

    std::ofstream f(path, std::ios::binary);
    f.write("RIFF", 4);
    f.write(reinterpret_cast<const char*>(&riffSize), 4);
    f.write("WAVE", 4);

    // fmt chunk
    f.write("fmt ", 4);
    uint32_t fmtSize = 16;
    f.write(reinterpret_cast<const char*>(&fmtSize), 4);
    uint16_t format = 1; // PCM
    f.write(reinterpret_cast<const char*>(&format), 2);
    f.write(reinterpret_cast<const char*>(&channels), 2);
    f.write(reinterpret_cast<const char*>(&sampleRate), 4);
    f.write(reinterpret_cast<const char*>(&byteRate), 4);
    f.write(reinterpret_cast<const char*>(&blockAlign), 2);
    f.write(reinterpret_cast<const char*>(&bitsPerSample), 2);

    // data chunk
    f.write("data", 4);
    f.write(reinterpret_cast<const char*>(&dataSize), 4);
    f.write(reinterpret_cast<const char*>(samples.data()),
             static_cast<std::streamsize>(dataSize));
}

} // anonymous

TEST_CASE("SoundBank: LoadWav reads 16-bit PCM mono", "[Audio]") {
    const std::string dir = "/tmp/nf_audio_test_" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    std::filesystem::create_directories(dir);
    const std::string path = dir + "/test.wav";

    // Generate a simple triangle wave: 100 samples at 44100 Hz.
    std::vector<int16_t> pcm(100);
    for (int i = 0; i < 100; ++i)
        pcm[i] = static_cast<int16_t>(i * 327);  // simple ramp

    WriteTestWav(path, 44100, 1, pcm);

    SoundBank bank;
    SoundId id = bank.LoadWav(path);
    REQUIRE(id != 0);

    const SoundAsset* asset = bank.Get(id);
    REQUIRE(asset != nullptr);
    REQUIRE(asset->SampleRate == 44100);
    REQUIRE(asset->Channels == 1);
    REQUIRE(asset->FrameCount() == 100);
    REQUIRE(asset->IsValid());

    // Verify first sample: 0 / 32768 ≈ 0.0
    REQUIRE(asset->Samples[0] == Approx(0.f).margin(0.001f));

    std::filesystem::remove_all(dir);
}

TEST_CASE("SoundBank: LoadWav returns 0 for missing file", "[Audio]") {
    SoundBank bank;
    SoundId id = bank.LoadWav("/tmp/nonexistent_file.wav");
    REQUIRE(id == 0);
}

TEST_CASE("SoundBank: LoadWav returns 0 for invalid format", "[Audio]") {
    const std::string dir = "/tmp/nf_audio_invalid_" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    std::filesystem::create_directories(dir);
    const std::string path = dir + "/bad.wav";

    // Write garbage that isn't a WAV.
    std::ofstream f(path, std::ios::binary);
    f << "NOT A WAV FILE";
    f.close();

    SoundBank bank;
    REQUIRE(bank.LoadWav(path) == 0);

    std::filesystem::remove_all(dir);
}

// ============================================================================
// AudioMixer — channel management
// ============================================================================

TEST_CASE("AudioMixer: Play creates an active channel", "[Audio]") {
    SoundBank bank;
    SoundId tone = bank.GenerateTone("tone", 440.f, 1.0f);

    AudioMixer mixer;
    mixer.SetSoundBank(&bank);

    ChannelHandle h = mixer.Play(tone, 0.8f, -0.5f);
    REQUIRE(h != 0);
    REQUIRE(mixer.ActiveChannelCount() == 1);

    const MixerChannel* ch = mixer.GetChannel(h);
    REQUIRE(ch != nullptr);
    REQUIRE(ch->SoundID == tone);
    REQUIRE(ch->Volume == Approx(0.8f));
    REQUIRE(ch->Pan == Approx(-0.5f));
    REQUIRE(ch->Playing);
    REQUIRE_FALSE(ch->Looping);
}

TEST_CASE("AudioMixer: Stop halts a channel", "[Audio]") {
    SoundBank bank;
    SoundId tone = bank.GenerateTone("tone", 440.f, 1.0f);

    AudioMixer mixer;
    mixer.SetSoundBank(&bank);

    ChannelHandle h = mixer.Play(tone);
    REQUIRE(mixer.ActiveChannelCount() == 1);

    mixer.Stop(h);
    REQUIRE(mixer.ActiveChannelCount() == 0);
}

TEST_CASE("AudioMixer: StopAll stops everything", "[Audio]") {
    SoundBank bank;
    SoundId t1 = bank.GenerateTone("a", 440.f, 1.0f);
    SoundId t2 = bank.GenerateTone("b", 880.f, 1.0f);

    AudioMixer mixer;
    mixer.SetSoundBank(&bank);
    mixer.Play(t1);
    mixer.Play(t2);
    REQUIRE(mixer.ActiveChannelCount() == 2);

    mixer.StopAll();
    REQUIRE(mixer.ActiveChannelCount() == 0);
}

TEST_CASE("AudioMixer: Update advances playback", "[Audio]") {
    SoundBank bank;
    SoundId tone = bank.GenerateTone("tone", 440.f, 1.0f);

    AudioMixer mixer;
    mixer.SetSoundBank(&bank);

    ChannelHandle h = mixer.Play(tone);
    mixer.Update(0.25f);

    const MixerChannel* ch = mixer.GetChannel(h);
    REQUIRE(ch != nullptr);
    REQUIRE(ch->PlaybackPos == Approx(0.25f));
    REQUIRE(ch->Playing);
}

TEST_CASE("AudioMixer: non-looping channel expires after duration", "[Audio]") {
    SoundBank bank;
    SoundId tone = bank.GenerateTone("tone", 440.f, 0.5f);

    AudioMixer mixer;
    mixer.SetSoundBank(&bank);

    ChannelHandle h = mixer.Play(tone, 1.f, 0.f, false);
    mixer.Update(0.6f);  // exceeds 0.5s duration

    // Channel should have been removed.
    REQUIRE(mixer.GetChannel(h) == nullptr);
    REQUIRE(mixer.ActiveChannelCount() == 0);
}

TEST_CASE("AudioMixer: looping channel wraps around", "[Audio]") {
    SoundBank bank;
    SoundId tone = bank.GenerateTone("tone", 440.f, 1.0f);

    AudioMixer mixer;
    mixer.SetSoundBank(&bank);

    ChannelHandle h = mixer.Play(tone, 1.f, 0.f, true);
    mixer.Update(1.3f);  // 0.3s into second loop

    const MixerChannel* ch = mixer.GetChannel(h);
    REQUIRE(ch != nullptr);
    REQUIRE(ch->Playing);
    REQUIRE(ch->PlaybackPos == Approx(0.3f).margin(0.01f));
}

TEST_CASE("AudioMixer: SetChannelVolume and SetChannelPan", "[Audio]") {
    SoundBank bank;
    SoundId tone = bank.GenerateTone("tone", 440.f, 1.0f);

    AudioMixer mixer;
    mixer.SetSoundBank(&bank);

    ChannelHandle h = mixer.Play(tone);
    mixer.SetChannelVolume(h, 0.3f);
    mixer.SetChannelPan(h, 0.7f);

    const MixerChannel* ch = mixer.GetChannel(h);
    REQUIRE(ch->Volume == Approx(0.3f));
    REQUIRE(ch->Pan == Approx(0.7f));
}

TEST_CASE("AudioMixer: channel limit enforced", "[Audio]") {
    SoundBank bank;
    SoundId tone = bank.GenerateTone("tone", 440.f, 1.0f);

    AudioMixer mixer;
    mixer.SetSoundBank(&bank);

    // Fill to max.
    for (int i = 0; i < AudioMixer::kMaxChannels; ++i)
        mixer.Play(tone, 1.f, 0.f, false, 200);

    REQUIRE(mixer.TotalChannelCount() == AudioMixer::kMaxChannels);

    // One more should still succeed (evicts lowest priority).
    ChannelHandle extra = mixer.Play(tone, 1.f, 0.f, false, 50);
    REQUIRE(extra != 0);
    REQUIRE(mixer.TotalChannelCount() <= AudioMixer::kMaxChannels);
}

// ============================================================================
// SpatialAudio — attenuation
// ============================================================================

TEST_CASE("SpatialAudio: ComputeAttenuation at min distance is 1.0", "[Audio]") {
    SpatialConfig cfg{1.f, 50.f, 1.f};
    REQUIRE(SpatialAudio::ComputeAttenuation(0.5f, cfg) == Approx(1.f));
    REQUIRE(SpatialAudio::ComputeAttenuation(1.0f, cfg) == Approx(1.f));
}

TEST_CASE("SpatialAudio: ComputeAttenuation at max distance is 0.0", "[Audio]") {
    SpatialConfig cfg{1.f, 50.f, 1.f};
    REQUIRE(SpatialAudio::ComputeAttenuation(50.f, cfg) == Approx(0.f));
    REQUIRE(SpatialAudio::ComputeAttenuation(100.f, cfg) == Approx(0.f));
}

TEST_CASE("SpatialAudio: ComputeAttenuation inverse distance", "[Audio]") {
    SpatialConfig cfg{1.f, 100.f, 1.f};
    // At distance 2: minDist / (minDist + rolloff*(dist-minDist))
    //              = 1 / (1 + 1*1) = 0.5
    REQUIRE(SpatialAudio::ComputeAttenuation(2.f, cfg) == Approx(0.5f));
    // At distance 5: 1 / (1 + 4) = 0.2
    REQUIRE(SpatialAudio::ComputeAttenuation(5.f, cfg) == Approx(0.2f));
}

TEST_CASE("SpatialAudio: ComputeAttenuation with rolloff factor 2", "[Audio]") {
    SpatialConfig cfg{1.f, 100.f, 2.f};
    // At distance 2: 1 / (1 + 2*1) = 1/3
    REQUIRE(SpatialAudio::ComputeAttenuation(2.f, cfg) == Approx(1.f / 3.f));
}

// ============================================================================
// SpatialAudio — stereo panning
// ============================================================================

TEST_CASE("SpatialAudio: pan is 0 for source on top of listener", "[Audio]") {
    SpatialAudio spatial;
    spatial.SetListenerPosition({0.f, 0.f, 0.f});
    spatial.SetListenerOrientation({0.f, 0.f, -1.f}, {0.f, 1.f, 0.f});

    REQUIRE(spatial.ComputePan({0.f, 0.f, 0.f}) == Approx(0.f));
}

TEST_CASE("SpatialAudio: pan is positive for source to the right", "[Audio]") {
    SpatialAudio spatial;
    spatial.SetListenerPosition({0.f, 0.f, 0.f});
    spatial.SetListenerOrientation({0.f, 0.f, -1.f}, {0.f, 1.f, 0.f});
    // Listener facing -Z, up +Y → right vector is -X (forward × up).
    // Actually: (0,0,-1) × (0,1,0) = (-1, 0, 0)? Let me check...
    // Cross product: (0*0 - (-1)*1, (-1)*0 - 0*0, 0*1 - 0*0) = (1, 0, 0)
    // Wait: a×b = (ay*bz - az*by, az*bx - ax*bz, ax*by - ay*bx)
    // forward=(0,0,-1), up=(0,1,0):
    //   rx = 0*0 - (-1)*1 = 1
    //   ry = (-1)*0 - 0*0 = 0
    //   rz = 0*1 - 0*0   = 0
    // So right = (1, 0, 0). Source at (10, 0, 0) → pan > 0.
    float pan = spatial.ComputePan({10.f, 0.f, 0.f});
    REQUIRE(pan > 0.f);
    REQUIRE(pan == Approx(1.f));
}

TEST_CASE("SpatialAudio: pan is negative for source to the left", "[Audio]") {
    SpatialAudio spatial;
    spatial.SetListenerPosition({0.f, 0.f, 0.f});
    spatial.SetListenerOrientation({0.f, 0.f, -1.f}, {0.f, 1.f, 0.f});

    float pan = spatial.ComputePan({-10.f, 0.f, 0.f});
    REQUIRE(pan < 0.f);
    REQUIRE(pan == Approx(-1.f));
}

TEST_CASE("SpatialAudio: pan is ~0 for source directly ahead", "[Audio]") {
    SpatialAudio spatial;
    spatial.SetListenerPosition({0.f, 0.f, 0.f});
    spatial.SetListenerOrientation({0.f, 0.f, -1.f}, {0.f, 1.f, 0.f});

    float pan = spatial.ComputePan({0.f, 0.f, -10.f});
    REQUIRE(pan == Approx(0.f).margin(0.01f));
}

// ============================================================================
// SpatialAudio — PlayAt integration
// ============================================================================

TEST_CASE("SpatialAudio: PlayAt creates a mixer channel with attenuation", "[Audio]") {
    SoundBank bank;
    SoundId tone = bank.GenerateTone("tone", 440.f, 1.0f);

    AudioMixer mixer;
    mixer.SetSoundBank(&bank);

    SpatialAudio spatial;
    spatial.SetMixer(&mixer);
    spatial.SetListenerPosition({0.f, 0.f, 0.f});
    spatial.SetListenerOrientation({0.f, 0.f, -1.f}, {0.f, 1.f, 0.f});

    SpatialConfig cfg{1.f, 50.f, 1.f};

    // Play at distance 2 → attenuation = 0.5
    ChannelHandle h = spatial.PlayAt(tone, {2.f, 0.f, 0.f}, 1.f, cfg);
    REQUIRE(h != 0);
    REQUIRE(mixer.ActiveChannelCount() == 1);

    const MixerChannel* ch = mixer.GetChannel(h);
    REQUIRE(ch != nullptr);
    REQUIRE(ch->Volume == Approx(0.5f));
    // Source at +X → pan should be positive (right).
    REQUIRE(ch->Pan > 0.f);
}

TEST_CASE("SpatialAudio: PlayAt returns 0 with no mixer", "[Audio]") {
    SpatialAudio spatial;
    REQUIRE(spatial.PlayAt(1, {0.f, 0.f, 0.f}) == 0);
}

// ============================================================================
// Orchestrator — Phase 9 integration
// ============================================================================

TEST_CASE("Orchestrator: Solo mode initialises audio subsystem", "[Audio]") {
    NF::Game::Orchestrator orch;
    orch.Init(nullptr);

    REQUIRE(orch.GetAudioDevice().IsInitialized());
    REQUIRE(orch.GetSoundBank().Count() == 3);  // mine, place, ambient

    // Verify placeholder sounds exist.
    REQUIRE(orch.GetSoundBank().GetByName("sfx_mine") != nullptr);
    REQUIRE(orch.GetSoundBank().GetByName("sfx_place") != nullptr);
    REQUIRE(orch.GetSoundBank().GetByName("sfx_ambient") != nullptr);

    // Tick should not crash.
    orch.Tick(1.f / 60.f);

    orch.Shutdown();
    REQUIRE_FALSE(orch.GetAudioDevice().IsInitialized());
    REQUIRE(orch.GetSoundBank().Count() == 0);
}

TEST_CASE("Orchestrator: audio mixer update runs per tick", "[Audio]") {
    NF::Game::Orchestrator orch;
    orch.Init(nullptr);

    // Play a sound through the mixer.
    const auto* asset = orch.GetSoundBank().GetByName("sfx_mine");
    REQUIRE(asset != nullptr);

    ChannelHandle h = orch.GetAudioMixer().Play(asset->Id, 0.5f);
    REQUIRE(h != 0);

    // Tick advances the mixer.
    orch.Tick(0.1f);

    const MixerChannel* ch = orch.GetAudioMixer().GetChannel(h);
    REQUIRE(ch != nullptr);
    REQUIRE(ch->PlaybackPos > 0.f);

    orch.Shutdown();
}
