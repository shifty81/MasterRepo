#pragma once
/**
 * @file HeadlessServer.h
 * @brief Embedded headless server runtime for offline AI + PCG processing.
 *
 * Runs the engine game loop, PCG pipeline, and AI orchestration without a
 * display window.  Suitable for batch asset generation, simulation playback,
 * dedicated multiplayer backends, and CI validation.
 *
 * Typical usage:
 * @code
 *   HeadlessServer server;
 *   HeadlessServerConfig cfg;
 *   cfg.tickRateHz = 30;
 *   cfg.enableAI   = true;
 *   cfg.enablePCG  = true;
 *   server.Init(cfg);
 *   server.LoadScene("Scenes/TestLevel.json");
 *   server.Run(600);  // run for up to 600 seconds
 *   server.Shutdown();
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

// ── Server configuration ──────────────────────────────────────────────────────

struct HeadlessServerConfig {
    uint32_t tickRateHz{30};          ///< simulation ticks per second
    bool     enableAI{true};          ///< run AI orchestration each tick
    bool     enablePCG{true};         ///< run PCG pipeline each tick
    bool     enableNetworking{false};  ///< listen for client connections
    uint16_t networkPort{7777};
    uint32_t maxClients{64};
    bool     verboseLogging{false};
    std::string logPath;              ///< empty = Logs/headless_server.log
};

// ── Runtime statistics ────────────────────────────────────────────────────────

struct HeadlessServerStats {
    uint64_t totalTicks{0};
    uint64_t uptimeMs{0};
    uint32_t connectedClients{0};
    float    avgTickMs{0.0f};
    float    peakTickMs{0.0f};
    uint64_t pcgAssetsGenerated{0};
    uint64_t aiRequestsHandled{0};
};

// ── HeadlessServer ────────────────────────────────────────────────────────────

class HeadlessServer {
public:
    HeadlessServer();
    ~HeadlessServer();

    void Init(const HeadlessServerConfig& config = {});
    void Shutdown();
    bool IsRunning() const;

    // ── Scene / world ─────────────────────────────────────────────────────────

    bool LoadScene(const std::string& scenePath);
    void UnloadScene();

    // ── Execution ─────────────────────────────────────────────────────────────

    /// Run for up to maxSeconds (0 = run until Stop() is called).
    void Run(uint32_t maxSeconds = 0);

    /// Request a graceful stop from another thread.
    void Stop();

    /// Execute a single tick (useful for unit tests).
    void TickOnce();

    // ── Stats ─────────────────────────────────────────────────────────────────

    HeadlessServerStats Stats() const;

    // ── Callbacks ─────────────────────────────────────────────────────────────

    void OnTick(std::function<void(uint64_t tickNumber)> cb);
    void OnClientConnected(std::function<void(uint32_t clientId)> cb);
    void OnClientDisconnected(std::function<void(uint32_t clientId)> cb);
    void OnError(std::function<void(const std::string& message)> cb);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Runtime
