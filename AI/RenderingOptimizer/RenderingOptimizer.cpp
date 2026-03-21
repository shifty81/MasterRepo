#include "AI/RenderingOptimizer/RenderingOptimizer.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <numeric>
#include <sstream>

namespace fs = std::filesystem;
namespace AI {

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

RenderingOptimizer::RenderingOptimizer(RenderingOptimizerConfig cfg)
    : m_cfg(std::move(cfg))
{}

// ─────────────────────────────────────────────────────────────────────────────
// Data ingestion
// ─────────────────────────────────────────────────────────────────────────────

void RenderingOptimizer::FeedFrame(const FrameMetrics& metrics)
{
    m_history.push_back(metrics);
    // Trim to smoothing window
    if ((int)m_history.size() > m_cfg.smoothingWindow)
        m_history.erase(m_history.begin());
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

FrameMetrics RenderingOptimizer::ComputeAverage() const
{
    if (m_history.empty()) return {};
    FrameMetrics avg{};
    for (const auto& f : m_history) {
        avg.frameTimeMs        += f.frameTimeMs;
        avg.gpuTimeMs          += f.gpuTimeMs;
        avg.cpuTimeMs          += f.cpuTimeMs;
        avg.drawCalls          += f.drawCalls;
        avg.triangleCount      += f.triangleCount;
        avg.particleCount      += f.particleCount;
        avg.textureMemoryMB    += f.textureMemoryMB;
        avg.vertexMemoryMB     += f.vertexMemoryMB;
        avg.lodGroupsActive    += f.lodGroupsActive;
    }
    float n = (float)m_history.size();
    avg.frameTimeMs     /= n;
    avg.gpuTimeMs       /= n;
    avg.cpuTimeMs       /= n;
    avg.drawCalls       = (int)((float)avg.drawCalls / n);
    avg.triangleCount   = (int)((float)avg.triangleCount / n);
    avg.particleCount   = (int)((float)avg.particleCount / n);
    avg.textureMemoryMB /= n;
    avg.vertexMemoryMB  /= n;
    avg.lodGroupsActive = (int)((float)avg.lodGroupsActive / n);
    avg.giEnabled           = m_history.back().giEnabled;
    avg.volumetricsEnabled  = m_history.back().volumetricsEnabled;
    avg.rayTracingEnabled   = m_history.back().rayTracingEnabled;
    avg.targetFPS           = m_cfg.targetFPS;
    return avg;
}

void RenderingOptimizer::CheckDrawCalls(const FrameMetrics& avg,
                                         std::vector<OptimSuggestion>& out) const
{
    if (avg.drawCalls > m_cfg.maxDrawCallsPerFrame) {
        OptimSuggestion s;
        s.category          = OptimCategory::DrawCall;
        s.priority          = (avg.drawCalls > m_cfg.maxDrawCallsPerFrame * 2)
                              ? OptimPriority::Critical : OptimPriority::High;
        s.title             = "High draw call count";
        s.description       = "Average " + std::to_string(avg.drawCalls) +
                              " draw calls per frame (budget: " +
                              std::to_string(m_cfg.maxDrawCallsPerFrame) + "). "
                              "Consider batching static meshes, using GPU instancing, "
                              "or frustum/occlusion culling.";
        s.action            = "reduce_draw_calls";
        s.estimatedGainMs   = (avg.drawCalls - m_cfg.maxDrawCallsPerFrame) * 0.002f;
        s.autoApplicable    = false;
        out.push_back(std::move(s));
    }
}

void RenderingOptimizer::CheckMemory(const FrameMetrics& avg,
                                      std::vector<OptimSuggestion>& out) const
{
    if (avg.textureMemoryMB > m_cfg.maxTextureMemoryMB) {
        OptimSuggestion s;
        s.category        = OptimCategory::Memory;
        s.priority        = OptimPriority::High;
        s.title           = "Texture memory over budget";
        s.description     = "Texture VRAM usage: " +
                            std::to_string((int)avg.textureMemoryMB) + " MB "
                            "(budget: " + std::to_string((int)m_cfg.maxTextureMemoryMB) +
                            " MB). Compress textures to DXT/BC formats or reduce "
                            "mip-map bias.";
        s.action          = "compress_textures";
        s.estimatedGainMs = 1.0f;
        s.autoApplicable  = false;
        out.push_back(std::move(s));
    }
}

void RenderingOptimizer::CheckParticles(const FrameMetrics& avg,
                                         std::vector<OptimSuggestion>& out) const
{
    if (avg.particleCount > m_cfg.maxParticlesPerFrame) {
        OptimSuggestion s;
        s.category        = OptimCategory::Particle;
        s.priority        = OptimPriority::Medium;
        s.title           = "Particle count exceeds budget";
        s.description     = std::to_string(avg.particleCount) +
                            " active particles (budget: " +
                            std::to_string(m_cfg.maxParticlesPerFrame) + "). "
                            "Reduce emitter rates or switch to GPU simulation.";
        s.action          = "reduce_particles";
        s.estimatedGainMs = 0.5f;
        s.autoApplicable  = false;
        out.push_back(std::move(s));
    }
}

void RenderingOptimizer::CheckGI(const FrameMetrics& avg,
                                  std::vector<OptimSuggestion>& out) const
{
    float targetMs = 1000.f / avg.targetFPS;
    if (avg.giEnabled && avg.frameTimeMs > targetMs * 1.5f) {
        OptimSuggestion s;
        s.category        = OptimCategory::GI;
        s.priority        = OptimPriority::Medium;
        s.title           = "GI contributing to frame time";
        s.description     = "Frame time is " + std::to_string((int)avg.frameTimeMs) +
                            " ms with GI enabled. Consider reducing probe density "
                            "or switching to static baked probes.";
        s.action          = "reduce_gi_probes";
        s.estimatedGainMs = avg.frameTimeMs * 0.15f;
        s.autoApplicable  = false;
        out.push_back(std::move(s));
    }
}

void RenderingOptimizer::CheckLOD(const FrameMetrics& avg,
                                   std::vector<OptimSuggestion>& out) const
{
    if (avg.lodGroupsActive == 0 && avg.triangleCount > 500000) {
        OptimSuggestion s;
        s.category        = OptimCategory::LOD;
        s.priority        = OptimPriority::High;
        s.title           = "LOD not active — high triangle count";
        s.description     = std::to_string(avg.triangleCount) +
                            " triangles per frame with no LOD groups registered. "
                            "Adding LOD can reduce triangle count by 40–70%.";
        s.action          = "enable_lod";
        s.estimatedGainMs = avg.gpuTimeMs * 0.3f;
        s.autoApplicable  = false;
        out.push_back(std::move(s));
    }
}

void RenderingOptimizer::CheckVolumetrics(const FrameMetrics& avg,
                                           std::vector<OptimSuggestion>& out) const
{
    float targetMs = 1000.f / avg.targetFPS;
    if (avg.volumetricsEnabled && avg.gpuTimeMs > targetMs * 0.5f) {
        OptimSuggestion s;
        s.category        = OptimCategory::PostProcess;
        s.priority        = OptimPriority::Low;
        s.title           = "Volumetric fog consuming GPU budget";
        s.description     = "Reduce froxel resolution or number of Z-slices "
                            "to recover GPU headroom.";
        s.action          = "reduce_volumetric_resolution";
        s.estimatedGainMs = avg.gpuTimeMs * 0.08f;
        s.autoApplicable  = false;
        out.push_back(std::move(s));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Public analysis API
// ─────────────────────────────────────────────────────────────────────────────

std::vector<OptimSuggestion> RenderingOptimizer::Analyse() const
{
    std::vector<OptimSuggestion> out;
    if (m_history.empty()) return out;

    FrameMetrics avg = ComputeAverage();
    CheckDrawCalls(avg, out);
    CheckMemory(avg, out);
    CheckParticles(avg, out);
    CheckGI(avg, out);
    CheckLOD(avg, out);
    CheckVolumetrics(avg, out);

    // Sort Critical → Low
    std::sort(out.begin(), out.end(),
              [](const OptimSuggestion& a, const OptimSuggestion& b){
                  return (int)a.priority > (int)b.priority;
              });

    // Optionally enrich with LLM explanation
    if (m_cfg.llmExplainFn) {
        for (auto& s : out) {
            std::string rich = m_cfg.llmExplainFn(s);
            if (!rich.empty()) s.description = rich;
        }
    }

    return out;
}

std::vector<OptimSuggestion> RenderingOptimizer::GetAutoSuggestions() const
{
    auto all = Analyse();
    std::vector<OptimSuggestion> result;
    for (auto& s : all)
        if (s.autoApplicable) result.push_back(std::move(s));
    return result;
}

std::vector<std::string> RenderingOptimizer::AutoApply()
{
    auto suggestions = GetAutoSuggestions();
    std::vector<std::string> applied;
    for (const auto& s : suggestions)
        applied.push_back(s.action);
    return applied;
}

// ─────────────────────────────────────────────────────────────────────────────
// Reporting
// ─────────────────────────────────────────────────────────────────────────────

std::string RenderingOptimizer::BuildReport() const
{
    std::ostringstream oss;
    oss << "# Atlas Engine — Rendering Performance Report\n\n";

    if (m_history.empty()) {
        oss << "*No frame data collected yet.*\n";
        return oss.str();
    }

    FrameMetrics avg = ComputeAverage();
    oss << "## Average Frame Stats (" << m_history.size() << " frames)\n\n"
        << "| Metric | Value |\n|--------|-------|\n"
        << "| Frame time | " << (int)avg.frameTimeMs << " ms |\n"
        << "| GPU time   | " << (int)avg.gpuTimeMs   << " ms |\n"
        << "| CPU time   | " << (int)avg.cpuTimeMs   << " ms |\n"
        << "| Draw calls | " << avg.drawCalls         << " |\n"
        << "| Triangles  | " << avg.triangleCount     << " |\n"
        << "| Particles  | " << avg.particleCount     << " |\n"
        << "| Tex VRAM   | " << (int)avg.textureMemoryMB << " MB |\n"
        << "\n## Suggestions\n\n";

    auto suggestions = Analyse();
    if (suggestions.empty()) {
        oss << "*No issues detected — target FPS maintained.*\n";
    } else {
        for (const auto& s : suggestions) {
            const char* prioStr =
                (s.priority == OptimPriority::Critical) ? "[CRIT]"   :
                (s.priority == OptimPriority::High)     ? "[HIGH]"   :
                (s.priority == OptimPriority::Medium)   ? "[MED]"    : "[LOW]";
            oss << "### " << prioStr << " — " << s.title << "\n"
                << s.description << "\n"
                << "**Action:** `" << s.action << "`"
                << " | **Est. gain:** " << s.estimatedGainMs << " ms\n\n";
        }
    }

    return oss.str();
}

bool RenderingOptimizer::WriteReport(const std::string& path) const
{
    try {
        fs::create_directories(fs::path(path).parent_path());
        std::ofstream ofs(path);
        if (!ofs) return false;
        ofs << BuildReport();
        return true;
    } catch (...) {
        return false;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Stats
// ─────────────────────────────────────────────────────────────────────────────

float RenderingOptimizer::AverageFrameTimeMs() const
{
    if (m_history.empty()) return 0.f;
    float s = 0.f;
    for (const auto& f : m_history) s += f.frameTimeMs;
    return s / (float)m_history.size();
}

float RenderingOptimizer::AverageGPUTimeMs() const
{
    if (m_history.empty()) return 0.f;
    float s = 0.f;
    for (const auto& f : m_history) s += f.gpuTimeMs;
    return s / (float)m_history.size();
}

float RenderingOptimizer::WorstFrameTimeMs() const
{
    if (m_history.empty()) return 0.f;
    float worst = 0.f;
    for (const auto& f : m_history)
        if (f.frameTimeMs > worst) worst = f.frameTimeMs;
    return worst;
}

int RenderingOptimizer::FrameCount() const
{
    return (int)m_history.size();
}

void RenderingOptimizer::Reset()
{
    m_history.clear();
}

} // namespace AI
