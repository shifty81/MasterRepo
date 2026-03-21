#include "Tools/BenchmarkRunner/BenchmarkRunner.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <numeric>

namespace Tools {

struct BenchmarkEntry {
    std::string   name;
    BenchmarkFn   fn;
};

struct BenchmarkRunner::Impl {
    std::vector<BenchmarkEntry> benchmarks;
};

BenchmarkRunner::BenchmarkRunner() : m_impl(new Impl()) {}
BenchmarkRunner::~BenchmarkRunner() { delete m_impl; }

void BenchmarkRunner::Register(const std::string& name, BenchmarkFn fn) {
    m_impl->benchmarks.push_back({name, std::move(fn)});
}

bool BenchmarkRunner::HasBenchmark(const std::string& name) const {
    for (const auto& b : m_impl->benchmarks) if (b.name == name) return true;
    return false;
}

size_t BenchmarkRunner::Count() const { return m_impl->benchmarks.size(); }

static BenchmarkResult RunEntry(const BenchmarkEntry& entry,
                                 const BenchmarkConfig& cfg)
{
    BenchmarkResult res;
    res.name       = entry.name;
    res.iterations = cfg.iterations;

    // Warmup
    for (uint32_t i = 0; i < cfg.warmupRuns; ++i) entry.fn();

    std::vector<double> timesUs;
    timesUs.reserve(cfg.iterations);

    using Clock = std::chrono::high_resolution_clock;
    for (uint32_t i = 0; i < cfg.iterations; ++i) {
        auto t0 = Clock::now();
        entry.fn();
        auto t1 = Clock::now();
        double us = static_cast<double>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count()) / 1000.0;
        timesUs.push_back(us);
    }

    std::sort(timesUs.begin(), timesUs.end());
    res.minUs    = timesUs.front();
    res.maxUs    = timesUs.back();
    res.meanUs   = std::accumulate(timesUs.begin(), timesUs.end(), 0.0) / timesUs.size();
    res.medianUs = timesUs[timesUs.size() / 2];

    double variance = 0.0;
    for (double t : timesUs) {
        double d = t - res.meanUs;
        variance += d * d;
    }
    res.stddevUs = std::sqrt(variance / static_cast<double>(timesUs.size()));
    res.totalMs  = std::accumulate(timesUs.begin(), timesUs.end(), 0.0) / 1000.0;
    return res;
}

BenchmarkResult BenchmarkRunner::RunOne(const std::string& name,
                                         const BenchmarkConfig& cfg)
{
    for (const auto& b : m_impl->benchmarks)
        if (b.name == name) return RunEntry(b, cfg);
    return {};
}

BenchmarkSuite BenchmarkRunner::RunAll(const BenchmarkConfig& cfg) {
    BenchmarkSuite suite;
    suite.config = cfg;
    suite.name   = "all";
    for (const auto& b : m_impl->benchmarks)
        suite.results.push_back(RunEntry(b, cfg));
    return suite;
}

BenchmarkSuite BenchmarkRunner::RunFilter(const std::string& filter,
                                           const BenchmarkConfig& cfg)
{
    BenchmarkSuite suite;
    suite.config = cfg;
    suite.name   = filter;
    for (const auto& b : m_impl->benchmarks) {
        if (b.name.find(filter) != std::string::npos)
            suite.results.push_back(RunEntry(b, cfg));
    }
    return suite;
}

std::string BenchmarkRunner::Export(const BenchmarkSuite& suite,
                                     ExportFormat fmt) const
{
    std::ostringstream ss;
    if (fmt == ExportFormat::CSV) {
        ss << "Name,Iterations,MinUs,MaxUs,MeanUs,MedianUs,StddevUs,TotalMs\n";
        for (const auto& r : suite.results) {
            ss << r.name << ',' << r.iterations << ','
               << std::fixed << std::setprecision(3)
               << r.minUs << ',' << r.maxUs << ',' << r.meanUs << ','
               << r.medianUs << ',' << r.stddevUs << ',' << r.totalMs << '\n';
        }
    } else if (fmt == ExportFormat::JSON) {
        ss << "[\n";
        for (size_t i = 0; i < suite.results.size(); ++i) {
            const auto& r = suite.results[i];
            ss << std::fixed << std::setprecision(3);
            ss << "  {\"name\":\"" << r.name << "\","
               << "\"iterations\":" << r.iterations << ","
               << "\"minUs\":"    << r.minUs    << ","
               << "\"maxUs\":"    << r.maxUs    << ","
               << "\"meanUs\":"   << r.meanUs   << ","
               << "\"medianUs\":" << r.medianUs << ","
               << "\"stddevUs\":" << r.stddevUs << ","
               << "\"totalMs\":"  << r.totalMs  << "}";
            if (i + 1 < suite.results.size()) ss << ',';
            ss << '\n';
        }
        ss << "]\n";
    } else { // Markdown
        ss << "| Benchmark | Iters | Min µs | Max µs | Mean µs | Median µs | Stddev µs | Total ms |\n";
        ss << "|-----------|------:|-------:|-------:|--------:|----------:|----------:|---------:|\n";
        for (const auto& r : suite.results) {
            ss << "| " << r.name << " | " << r.iterations << " | "
               << std::fixed << std::setprecision(1)
               << r.minUs << " | " << r.maxUs << " | " << r.meanUs << " | "
               << r.medianUs << " | " << r.stddevUs << " | "
               << std::setprecision(2) << r.totalMs << " |\n";
        }
    }
    return ss.str();
}

bool BenchmarkRunner::SaveToFile(const BenchmarkSuite& suite,
                                  const std::string& path,
                                  ExportFormat fmt) const
{
    std::ofstream f(path);
    if (!f) return false;
    f << Export(suite, fmt);
    return true;
}

std::string BenchmarkRunner::Compare(const BenchmarkSuite& baseline,
                                      const BenchmarkSuite& current) const
{
    std::ostringstream ss;
    ss << "| Benchmark | Baseline µs | Current µs | Delta % |\n";
    ss << "|-----------|------------:|-----------:|--------:|\n";
    for (const auto& cur : current.results) {
        double baseMean = 0.0;
        for (const auto& b : baseline.results)
            if (b.name == cur.name) { baseMean = b.meanUs; break; }
        double delta = baseMean > 0.0
            ? (cur.meanUs - baseMean) / baseMean * 100.0 : 0.0;
        ss << "| " << cur.name << " | "
           << std::fixed << std::setprecision(1)
           << baseMean << " | " << cur.meanUs << " | "
           << std::setprecision(1) << delta << "% |\n";
    }
    return ss.str();
}

} // namespace Tools
