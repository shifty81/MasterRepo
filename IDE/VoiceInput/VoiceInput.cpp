#include "IDE/VoiceInput/VoiceInput.h"
#include <algorithm>
#include <cctype>

// Stub implementation — integrates with Whisper / Vosk / Voxtral at runtime.
// The public API is stable; only the Impl internals change when a real STT
// backend is wired in.

namespace IDE {

struct VoiceInput::Impl {
    bool                      listening{false};
    float                     threshold{0.6f};
    std::string               lastTranscript;
    std::vector<VoiceCommand> commands;
    TranscriptCallback        onTranscript;
    ActionCallback            onAction;
};

VoiceInput::VoiceInput() : m_impl(new Impl()) {}
VoiceInput::~VoiceInput() { Stop(); delete m_impl; }

bool VoiceInput::Start() {
    if (m_impl->listening) return true;
    // Real backend init (Whisper / Vosk) goes here.
    m_impl->listening = true;
    return true;
}

void VoiceInput::Stop() {
    if (!m_impl->listening) return;
    m_impl->listening = false;
}

bool VoiceInput::IsListening() const { return m_impl->listening; }

void VoiceInput::RegisterCommand(const VoiceCommand& cmd) {
    m_impl->commands.push_back(cmd);
}

void VoiceInput::ClearCommands() { m_impl->commands.clear(); }

void VoiceInput::SetConfidenceThreshold(float t) { m_impl->threshold = t; }

void VoiceInput::OnTranscript(TranscriptCallback cb) {
    m_impl->onTranscript = std::move(cb);
}

void VoiceInput::OnAction(ActionCallback cb) {
    m_impl->onAction = std::move(cb);
}

std::vector<VoiceCommand> VoiceInput::GetCommands() const {
    return m_impl->commands;
}

std::string VoiceInput::LastTranscript() const { return m_impl->lastTranscript; }

} // namespace IDE
