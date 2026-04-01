#pragma once
#include <cstdint>
#include <deque>
#include <string>
#include <vector>

namespace NF::Editor {

/// @brief Single profiler sample (one frame).
struct ProfilerSample {
    double cpuMs{0.0};   ///< CPU frame time in milliseconds.
    double gpuMs{0.0};   ///< GPU frame time in milliseconds (0 if unavailable).
    double totalMs{0.0}; ///< Wall-clock frame time.
};

/// @brief Named CPU scope marker captured within a frame.
struct ProfilerScope {
    std::string name;
    double      durationMs{0.0};
    uint32_t    depth{0}; ///< Nesting depth (0 = root).
};

/// @brief Live CPU/GPU frame-time profiler backend.
///
/// Call BeginFrame()/EndFrame() each game frame to record timing.
/// Use PushScope()/PopScope() inside systems to capture named scopes.
/// The panel reads sample history via GetSamples() for visualisation.
class LiveProfilerBackend {
public:
    static constexpr uint32_t kHistorySize = 256;

    /// @brief Start of a new frame – captures wall-clock start time.
    void BeginFrame();

    /// @brief End of frame – finalises CPU timing and stores the sample.
    void EndFrame();

    /// @brief Provide a GPU frame time from an external query (e.g. GL timer query).
    /// @param gpuMs GPU render time for the current frame in milliseconds.
    void SetGpuTime(double gpuMs) noexcept { m_PendingGpuMs = gpuMs; }

    /// @brief Mark the start of a named CPU scope inside a frame.
    void PushScope(const std::string& name);

    /// @brief Mark the end of the most recently pushed scope.
    void PopScope();

    /// @brief Return the rolling history of recorded frame samples.
    [[nodiscard]] const std::deque<ProfilerSample>& GetSamples() const noexcept { return m_Samples; }

    /// @brief Return scopes captured during the most recently completed frame.
    [[nodiscard]] const std::vector<ProfilerScope>& GetLastFrameScopes() const noexcept { return m_LastScopes; }

    /// @brief Average CPU frame time over the last N samples (ms).
    [[nodiscard]] double AverageCpuMs(uint32_t n = 60) const noexcept;

    /// @brief Average GPU frame time over the last N samples (ms).
    [[nodiscard]] double AverageGpuMs(uint32_t n = 60) const noexcept;

    /// @brief Peak CPU frame time in the current history window (ms).
    [[nodiscard]] double PeakCpuMs() const noexcept;

private:
    struct ScopeEntry {
        std::string name;
        double      startMs{0.0};
        uint32_t    depth{0};
    };

    std::deque<ProfilerSample>  m_Samples;
    std::vector<ProfilerScope>  m_LastScopes;
    std::vector<ProfilerScope>  m_CurrentScopes;
    std::vector<ScopeEntry>     m_ScopeStack;

    double   m_FrameStartMs{0.0};
    double   m_PendingGpuMs{0.0};

    static double NowMs() noexcept;
};

} // namespace NF::Editor
