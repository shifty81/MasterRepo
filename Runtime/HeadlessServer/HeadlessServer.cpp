#include "Runtime/HeadlessServer/HeadlessServer.h"
#include <atomic>
#include <chrono>
#include <thread>

namespace Runtime {

static uint64_t NowMs() {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
}

struct HeadlessServer::Impl {
    HeadlessServerConfig config;
    HeadlessServerStats  stats;
    std::atomic<bool>    running{false};
    std::atomic<bool>    stopRequested{false};
    std::string          loadedScene;

    std::function<void(uint64_t)>       onTick;
    std::function<void(uint32_t)>       onClientConnected;
    std::function<void(uint32_t)>       onClientDisconnected;
    std::function<void(const std::string&)> onError;
};

HeadlessServer::HeadlessServer() : m_impl(new Impl()) {}
HeadlessServer::~HeadlessServer() {
    if (m_impl->running) Stop();
    delete m_impl;
}

void HeadlessServer::Init(const HeadlessServerConfig& config) {
    m_impl->config = config;
    m_impl->stats  = {};
}

void HeadlessServer::Shutdown() {
    Stop();
    m_impl->loadedScene.clear();
}

bool HeadlessServer::IsRunning() const { return m_impl->running; }

bool HeadlessServer::LoadScene(const std::string& scenePath) {
    m_impl->loadedScene = scenePath;
    return true;
}

void HeadlessServer::UnloadScene() { m_impl->loadedScene.clear(); }

void HeadlessServer::Run(uint32_t maxSeconds) {
    m_impl->running       = true;
    m_impl->stopRequested = false;
    m_impl->stats.totalTicks = 0;

    const uint32_t tickIntervalMs =
        m_impl->config.tickRateHz > 0
        ? (1000u / m_impl->config.tickRateHz) : 33u;

    uint64_t startMs = NowMs();
    uint64_t endMs   = maxSeconds > 0 ? startMs + maxSeconds * 1000ULL : 0;

    while (!m_impl->stopRequested) {
        if (endMs > 0 && NowMs() >= endMs) break;

        uint64_t tickStart = NowMs();
        TickOnce();
        uint64_t elapsed = NowMs() - tickStart;

        float ms = static_cast<float>(elapsed);
        m_impl->stats.peakTickMs = std::max(m_impl->stats.peakTickMs, ms);
        m_impl->stats.avgTickMs  =
            (m_impl->stats.avgTickMs * (m_impl->stats.totalTicks - 1) + ms)
            / static_cast<float>(m_impl->stats.totalTicks);

        if (elapsed < tickIntervalMs) {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(tickIntervalMs - elapsed));
        }
    }

    m_impl->stats.uptimeMs = NowMs() - startMs;
    m_impl->running        = false;
}

void HeadlessServer::Stop() { m_impl->stopRequested = true; }

void HeadlessServer::TickOnce() {
    ++m_impl->stats.totalTicks;
    if (m_impl->onTick) m_impl->onTick(m_impl->stats.totalTicks);
}

HeadlessServerStats HeadlessServer::Stats() const { return m_impl->stats; }

void HeadlessServer::OnTick(std::function<void(uint64_t)> cb) {
    m_impl->onTick = std::move(cb);
}

void HeadlessServer::OnClientConnected(std::function<void(uint32_t)> cb) {
    m_impl->onClientConnected = std::move(cb);
}

void HeadlessServer::OnClientDisconnected(std::function<void(uint32_t)> cb) {
    m_impl->onClientDisconnected = std::move(cb);
}

void HeadlessServer::OnError(std::function<void(const std::string&)> cb) {
    m_impl->onError = std::move(cb);
}

} // namespace Runtime
