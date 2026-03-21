#include "Editor/Viewport/VRPreview.h"
#include <string>

namespace Editor {

// ─────────────────────────────────────────────────────────────────────────────
// Internal implementation
// ─────────────────────────────────────────────────────────────────────────────

struct VRPreview::Impl {
    XRMode   mode    = XRMode::None;
    bool     running = false;
    float    simIPD  = 0.064f;          // metres
    std::string runtimeName = "None";

    // Simulated pose state
    std::array<float,3> simHeadPos = {0.f, 1.6f, 0.f};
    std::array<float,4> simHeadRot = {0.f, 0.f, 0.f, 1.f};

    // Controller state (all zeroed by default)
    ControllerState leftCtrl;
    ControllerState rightCtrl;

    VRPreview::SessionChangedFn onSessionChanged;
};

// ─────────────────────────────────────────────────────────────────────────────
// Construction / destruction
// ─────────────────────────────────────────────────────────────────────────────

VRPreview::VRPreview()
    : m_impl(new Impl())
{}

VRPreview::~VRPreview()
{
    Shutdown();
    delete m_impl;
}

// ─────────────────────────────────────────────────────────────────────────────
// Init / Shutdown
// ─────────────────────────────────────────────────────────────────────────────

bool VRPreview::Init(XRMode preferredMode)
{
    if (m_impl->running) return true;

    // OpenXR runtime discovery stub.
    // In a production build this would call xrEnumerateApiLayerProperties /
    // xrCreateInstance / xrGetSystem.  Without the OpenXR SDK installed we
    // unconditionally fall back to Simulated mode.

    bool xrAvailable = false; // Set to true when OpenXR SDK is linked

    if (!xrAvailable || preferredMode == XRMode::Simulated) {
        m_impl->mode        = XRMode::Simulated;
        m_impl->runtimeName = "Simulated HMD";
    } else {
        m_impl->mode        = preferredMode;
        m_impl->runtimeName = "OpenXR Runtime";
    }

    m_impl->running = true;

    if (m_impl->onSessionChanged)
        m_impl->onSessionChanged(true);

    return true;
}

void VRPreview::Shutdown()
{
    if (!m_impl->running) return;
    m_impl->running = false;
    m_impl->mode    = XRMode::None;

    if (m_impl->onSessionChanged)
        m_impl->onSessionChanged(false);
}

bool      VRPreview::IsRunning()      const { return m_impl->running; }
XRMode    VRPreview::GetMode()        const { return m_impl->mode; }
std::string VRPreview::GetRuntimeName() const { return m_impl->runtimeName; }

// ─────────────────────────────────────────────────────────────────────────────
// Per-frame
// ─────────────────────────────────────────────────────────────────────────────

void VRPreview::PollEvents()
{
    // In a real implementation: call xrPollEvent in a loop, handling
    // XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED, etc.
    // For the simulated backend nothing needs to be done here.
}

bool VRPreview::BeginFrame(XRFrameInfo& info)
{
    if (!m_impl->running) return false;

    // Build synthetic view info for the simulated / stub backend
    float halfIPD = m_impl->simIPD * 0.5f;

    info.leftEye.position  = { m_impl->simHeadPos[0] - halfIPD,
                                m_impl->simHeadPos[1],
                                m_impl->simHeadPos[2] };
    info.leftEye.orientation  = m_impl->simHeadRot;

    info.rightEye.position = { m_impl->simHeadPos[0] + halfIPD,
                                m_impl->simHeadPos[1],
                                m_impl->simHeadPos[2] };
    info.rightEye.orientation = m_impl->simHeadRot;

    info.predictedDisplayTime = 0; // real impl: xrWaitFrame result

    return true;
}

void VRPreview::EndFrame(uint32_t /*leftTexId*/, uint32_t /*rightTexId*/)
{
    // Real impl: xrEndFrame with XrCompositionLayerProjection.
    // Simulated: nothing to blit to hardware.
}

// ─────────────────────────────────────────────────────────────────────────────
// Controllers
// ─────────────────────────────────────────────────────────────────────────────

VRPreview::ControllerState VRPreview::GetLeftController()  const
{
    return m_impl->leftCtrl;
}

VRPreview::ControllerState VRPreview::GetRightController() const
{
    return m_impl->rightCtrl;
}

// ─────────────────────────────────────────────────────────────────────────────
// Callbacks
// ─────────────────────────────────────────────────────────────────────────────

void VRPreview::SetSessionChangedCallback(SessionChangedFn cb)
{
    m_impl->onSessionChanged = std::move(cb);
}

// ─────────────────────────────────────────────────────────────────────────────
// Simulated HMD helpers
// ─────────────────────────────────────────────────────────────────────────────

void VRPreview::SetSimulatedIPD(float ipd)
{
    m_impl->simIPD = (ipd > 0.f) ? ipd : 0.064f;
}

void VRPreview::SetSimulatedHeadPose(const std::array<float,3>& pos,
                                      const std::array<float,4>& rot)
{
    m_impl->simHeadPos = pos;
    m_impl->simHeadRot = rot;
}

} // namespace Editor
