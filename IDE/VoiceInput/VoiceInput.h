#pragma once
#include <string>
#include <vector>
#include <functional>

namespace IDE {

/// Recognition result returned by the voice backend.
struct VoiceResult {
    std::string text;       ///< Transcribed text
    float       confidence; ///< 0.0 – 1.0
    bool        isFinal;    ///< False while still processing
};

/// A mapped voice command: pattern → action name.
struct VoiceCommand {
    std::string pattern;    ///< Keyword / phrase to match (case-insensitive)
    std::string actionName; ///< Name of the editor action to fire
};

using TranscriptCallback = std::function<void(const VoiceResult&)>;
using ActionCallback     = std::function<void(const std::string& actionName,
                                              const VoiceResult& result)>;

/// VoiceInput — voice-to-action pipeline for the editor.
///
/// Flow: microphone → backend transcription → command matching → action dispatch.
/// In offline mode the backend is a stub; swap in Whisper / Vosk for real STT.
class VoiceInput {
public:
    VoiceInput();
    ~VoiceInput();

    // ── lifecycle ──────────────────────────────────────────────
    /// Initialise the audio backend and start listening.
    bool Start();
    /// Stop listening and release audio resources.
    void Stop();
    bool IsListening() const;

    // ── configuration ─────────────────────────────────────────
    /// Register a keyword mapping.  Multiple commands can be registered.
    void RegisterCommand(const VoiceCommand& cmd);
    void ClearCommands();

    /// Minimum confidence threshold to accept a result (default 0.6).
    void SetConfidenceThreshold(float threshold);

    // ── callbacks ─────────────────────────────────────────────
    /// Called for every transcript update (including partial results).
    void OnTranscript(TranscriptCallback cb);
    /// Called when a registered command is recognised.
    void OnAction(ActionCallback cb);

    // ── query ─────────────────────────────────────────────────
    std::vector<VoiceCommand> GetCommands() const;
    std::string               LastTranscript() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace IDE
