#pragma once
/**
 * @file BenchmarkRunner.h
 * @brief Micro-benchmark runner for engine and AI subsystems.
 *
 * BenchmarkRunner provides a lightweight framework for measuring and
 * comparing performance of individual subsystem operations:
 *   - Register named benchmarks (each is a callable)
 *   - Run with configurable iteration count and warmup
 *   - Collect per-benchmark stats: min/max/mean/median/stddev
 *   - Export results as CSV, JSON, or Markdown table
 *
 * Intended for the nightly CI pipeline (Tools/TestPipeline) and for
 * ad-hoc profiling during development.
 */

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <optional>

namespace Tools {

// ── Benchmark registration ────────────────────────────────────────────────
using BenchmarkFn = std::function<void()>;

struct BenchmarkConfig {
    uint32_t warmupRuns{5};
    uint32_t iterations{100};
    bool     printProgress{false};
};

// ── Result ────────────────────────────────────────────────────────────────
struct BenchmarkResult {
    std::string name;
    uint32_t    iterations{0};
    double      minUs{0.0};
    double      maxUs{0.0};
    double      meanUs{0.0};
    double      medianUs{0.0};
    double      stddevUs{0.0};
    double      totalMs{0.0};
};

// ── Suite ─────────────────────────────────────────────────────────────────
struct BenchmarkSuite {
    std::string                  name;
    BenchmarkConfig              config;
    std::vector<BenchmarkResult> results;
    bool                         passed{true};
};

enum class ExportFormat { CSV, JSON, Markdown };

/// BenchmarkRunner — register, run, and export micro-benchmark results.
class BenchmarkRunner {
public:
    BenchmarkRunner();
    ~BenchmarkRunner();

    // ── registration ──────────────────────────────────────────
    void Register(const std::string& name, BenchmarkFn fn);
    bool HasBenchmark(const std::string& name) const;
    size_t Count() const;

    // ── execution ─────────────────────────────────────────────
    BenchmarkSuite RunAll(const BenchmarkConfig& cfg = {});
    BenchmarkResult RunOne(const std::string& name,
                           const BenchmarkConfig& cfg = {});

    // ── filtering ─────────────────────────────────────────────
    /// Run only benchmarks whose names contain the given filter string.
    BenchmarkSuite RunFilter(const std::string& filter,
                             const BenchmarkConfig& cfg = {});

    // ── export ────────────────────────────────────────────────
    std::string Export(const BenchmarkSuite& suite,
                       ExportFormat fmt = ExportFormat::Markdown) const;
    bool SaveToFile(const BenchmarkSuite& suite,
                    const std::string& path,
                    ExportFormat fmt = ExportFormat::Markdown) const;

    // ── comparison ────────────────────────────────────────────
    /// Compare two suites; returns a diff table as a string.
    std::string Compare(const BenchmarkSuite& baseline,
                        const BenchmarkSuite& current) const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Tools
