#pragma once
#include <functional>
#include <string>
#include "Core/PluginSystem/IPlugin.h"

namespace Plugins {

// ──────────────────────────────────────────────────────────────
// IAIPlugin — interface for AI model / inference plugins
// ──────────────────────────────────────────────────────────────
//
// Implementors wrap an offline language model backend (e.g. llama.cpp,
// Ollama HTTP API) and expose a uniform inference surface to the engine.
//
// Usage:
//   class MyLlamaPlugin : public Plugins::IAIPlugin {
//   public:
//       Core::PluginSystem::PluginInfo GetInfo() const override { ... }
//       bool OnLoad()   override { return Load(); }
//       void OnUnload() override { Unload(); }
//       // ... implement pure virtuals ...
//   };

class IAIPlugin : public Core::PluginSystem::IPlugin {
public:
    static constexpr const char* PluginType = "AI";

    ~IAIPlugin() override = default;

    // ── Model identity ───────────────────────────────────────
    [[nodiscard]] virtual std::string GetModelName() const = 0;
    [[nodiscard]] virtual std::string GetModelPath() const = 0;

    // ── Lifecycle ────────────────────────────────────────────
    [[nodiscard]] virtual bool IsLoaded() const = 0;
    virtual bool Load()   = 0;
    virtual bool Unload() = 0;

    // ── Inference ────────────────────────────────────────────
    /// Blocking single-shot inference; returns the full completion string.
    virtual std::string Infer(const std::string& prompt, int maxTokens) = 0;

    /// Streaming inference: calls cb once per token as they arrive.
    /// Returns false if the model is not loaded or streaming is unsupported.
    virtual bool StreamInfer(const std::string& prompt,
                             std::function<void(const std::string& token)> cb) = 0;

    // ── Capabilities ─────────────────────────────────────────
    /// Maximum number of tokens the model can hold in its context window.
    [[nodiscard]] virtual int GetContextWindow() const = 0;
};

} // namespace Plugins
