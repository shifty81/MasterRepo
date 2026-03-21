#pragma once
// VRPreview — Phase 10.10: VR/AR preview mode in the editor viewport (OpenXR)
//
// Wraps an OpenXR session lifecycle so the editor viewport can stream its
// rendered frame to an HMD or AR overlay without requiring the full game
// runtime.  When no XR runtime is installed the system degrades gracefully
// to a flat "simulated HMD" preview window.

#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include <array>

namespace Editor {

// ─────────────────────────────────────────────────────────────────────────────
// XR eye / view types
// ─────────────────────────────────────────────────────────────────────────────

enum class XRMode {
    None,       // VR/AR disabled
    VR,         // Stereo VR (e.g. Oculus, Index, WMR)
    AR,         // Passthrough AR (e.g. Magic Leap, HoloLens stub)
    Simulated,  // Flat "split-screen" HMD simulator (no hardware needed)
};

struct XRViewInfo {
    float fovLeft   = -0.7854f; // radians
    float fovRight  =  0.7854f;
    float fovUp     =  0.7854f;
    float fovDown   = -0.7854f;
    float nearZ     =  0.05f;
    float farZ      =  1000.f;
    std::array<float, 3> position = {0.f, 0.f, 0.f};  // head-space pose
    std::array<float, 4> orientation = {0.f, 0.f, 0.f, 1.f}; // quaternion xyzw
};

struct XRFrameInfo {
    XRViewInfo  leftEye;
    XRViewInfo  rightEye;
    uint64_t    predictedDisplayTime = 0; // nanoseconds
};

// ─────────────────────────────────────────────────────────────────────────────
// VRPreview
// ─────────────────────────────────────────────────────────────────────────────

class VRPreview {
public:
    VRPreview();
    ~VRPreview();

    // ── Lifecycle ────────────────────────────────────────────────────────────

    /// Attempt to initialise an XR runtime.
    /// @param preferredMode   Hint; falls back to Simulated if unavailable.
    /// @return true if any XR mode is now active.
    bool Init(XRMode preferredMode = XRMode::VR);

    /// Shut down the XR session and release GPU resources.
    void Shutdown();

    bool IsRunning()     const;
    XRMode GetMode()     const;
    std::string GetRuntimeName() const;

    // ── Per-frame ────────────────────────────────────────────────────────────

    /// Poll XR events (must be called every frame even if not rendering).
    void PollEvents();

    /// Begin an XR frame.  Fills *info* with per-eye view data.
    /// Returns false if the frame should be skipped (e.g. session not focused).
    bool BeginFrame(XRFrameInfo& info);

    /// Submit rendered eye textures.
    /// @param leftTexId / rightTexId   OpenGL / Vulkan texture handles.
    void EndFrame(uint32_t leftTexId, uint32_t rightTexId);

    // ── Controller input ─────────────────────────────────────────────────────

    struct ControllerState {
        std::array<float, 3> position    = {};
        std::array<float, 4> orientation = {0.f, 0.f, 0.f, 1.f};
        float  triggerValue = 0.f;
        float  gripValue    = 0.f;
        bool   buttonA      = false;
        bool   buttonB      = false;
        bool   thumbstickClick = false;
        std::array<float, 2> thumbstick = {};
    };

    ControllerState GetLeftController()  const;
    ControllerState GetRightController() const;

    // ── Callbacks ────────────────────────────────────────────────────────────

    using SessionChangedFn = std::function<void(bool active)>;
    void SetSessionChangedCallback(SessionChangedFn cb);

    // ── Simulated HMD helpers ────────────────────────────────────────────────

    /// In Simulated mode, override eye separation (default 0.064 m).
    void SetSimulatedIPD(float ipd);

    /// In Simulated mode, set a mock head-pose (for testing).
    void SetSimulatedHeadPose(const std::array<float,3>& pos,
                              const std::array<float,4>& rot);

private:
    struct Impl;
    Impl* m_impl = nullptr;
};

} // namespace Editor
