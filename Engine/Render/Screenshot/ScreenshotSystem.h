#pragma once
/**
 * @file ScreenshotSystem.h
 * @brief Captures screenshots and video frames from the rendered framebuffer.
 *
 * Supports:
 *   - Single-frame PNG / JPEG capture
 *   - Supersampled (high-res) captures (render at 2×/4× then downsample)
 *   - Frame sequence capture (for video export)
 *   - Async save to disk (non-blocking)
 *   - Overlay suppression (HUD-off mode)
 *   - Custom output directory and filename templates
 *
 * Typical usage:
 * @code
 *   ScreenshotSystem ss;
 *   ss.Init();
 *   ss.SetOutputDir("Screenshots/");
 *   ss.Capture();                // saves next frame
 *   ss.CaptureHiRes(4);          // 4× supersampling
 *   ss.BeginSequence(60);        // capture 60 frames
 *   ss.Tick(dt);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

enum class ImageFormat : uint8_t { PNG=0, JPEG, BMP };

struct CaptureRequest {
    std::string   filename;         ///< empty = auto-generated
    ImageFormat   format{ImageFormat::PNG};
    uint32_t      supersample{1};   ///< 1=normal, 2=2x, 4=4x
    bool          hideHUD{false};
    bool          async{true};
};

struct CaptureResult {
    std::string path;
    uint32_t    width{0}, height{0};
    bool        succeeded{false};
};

class ScreenshotSystem {
public:
    ScreenshotSystem();
    ~ScreenshotSystem();

    void Init();
    void Shutdown();

    void SetOutputDir(const std::string& dir);
    void SetFilenameTemplate(const std::string& tmpl); ///< e.g. "shot_{date}_{index}"

    // Single capture
    void Capture(const CaptureRequest& req = {});

    // Supersampled
    void CaptureHiRes(uint32_t scale = 2, ImageFormat fmt = ImageFormat::PNG);

    // Sequence (for video)
    void BeginSequence(uint32_t frameCount, uint32_t fps = 60);
    void EndSequence();
    bool IsCapturingSequence() const;
    uint32_t SequenceFramesRemaining() const;

    // Per-frame hook (call from render loop)
    void Tick(float dt);

    // Query pending
    uint32_t PendingCount() const;

    // Callbacks
    void OnCaptureDone(std::function<void(const CaptureResult&)> cb);
    void OnSequenceDone(std::function<void(const std::string& outputDir, uint32_t frames)> cb);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Engine
