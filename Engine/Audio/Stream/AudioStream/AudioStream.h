#pragma once
/**
 * @file AudioStream.h
 * @brief PCM audio stream: decode-on-demand, seek, loop, format conversion, callback feed.
 *
 * Features:
 *   - OpenFile(path) → bool: open an audio file for streaming (PCM/WAV)
 *   - OpenMemory(data, size) → bool: open from in-memory buffer
 *   - Close()
 *   - Play() / Pause() / Stop()
 *   - Seek(sampleOffset) → bool
 *   - GetPositionSamples() → uint64_t
 *   - GetDurationSamples() → uint64_t
 *   - GetSampleRate() → uint32_t
 *   - GetChannelCount() → uint32_t
 *   - GetBitsPerSample() → uint32_t
 *   - ReadSamples(outPCM, frameCount) → uint32_t: decode up to frameCount frames
 *   - SetLooping(on) / IsLooping() → bool
 *   - SetVolume(v) / GetVolume() → float
 *   - SetPitch(p) / GetPitch() → float
 *   - SetOnEnd(cb): callback when stream reaches end (non-looping)
 *   - SetOnSeek(cb): callback after Seek()
 *   - IsPlaying() / IsPaused() / IsAtEnd() → bool
 *   - SetFeedCallback(cb): per-frame pull callback(outPCM, frameCount)
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

class AudioStream {
public:
    AudioStream();
    ~AudioStream();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Open/Close
    bool OpenFile  (const std::string& path);
    bool OpenMemory(const uint8_t* data, uint32_t size);
    void Close     ();

    // Playback control
    void Play ();
    void Pause();
    void Stop ();

    // Position
    bool     Seek              (uint64_t sampleOffset);
    uint64_t GetPositionSamples() const;
    uint64_t GetDurationSamples() const;

    // Format info
    uint32_t GetSampleRate    () const;
    uint32_t GetChannelCount  () const;
    uint32_t GetBitsPerSample () const;

    // Decoding
    uint32_t ReadSamples(float* outPCM, uint32_t frameCount);

    // Settings
    void  SetLooping(bool on);
    bool  IsLooping () const;
    void  SetVolume (float v);
    float GetVolume () const;
    void  SetPitch  (float p);
    float GetPitch  () const;

    // State
    bool IsPlaying() const;
    bool IsPaused () const;
    bool IsAtEnd  () const;

    // Callbacks
    void SetOnEnd (std::function<void()> cb);
    void SetOnSeek(std::function<void(uint64_t)> cb);
    void SetFeedCallback(std::function<void(float*, uint32_t)> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
