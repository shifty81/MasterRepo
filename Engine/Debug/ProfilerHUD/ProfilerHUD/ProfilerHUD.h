#pragma once
/**
 * @file ProfilerHUD.h
 * @brief In-engine profiler: named timing scopes, per-frame bar graphs, history, and JSON export.
 *
 * Features:
 *   - BeginScope(name) / EndScope(name): bracket CPU timing scopes
 *   - RAII helper: ScopeGuard(profiler, name)
 *   - GetScopeMs(name) → float: last frame time for scope in milliseconds
 *   - GetScopeHistory(name, outMs, count): rolling history buffer
 *   - GetAllScopes() → vector<ScopeInfo>: name + last ms + avg ms + max ms
 *   - SetBudgetMs(name, ms): mark scope red when it exceeds budget
 *   - NewFrame(): advance frame counter, rotate history
 *   - GetFrameMs() → float: total last-frame CPU time
 *   - GetFPS() → float: smoothed frames per second
 *   - RenderBars(outLines[]): emit ASCII/line draw data for HUD overlay
 *   - ExportJSON() → std::string: full history dump
 *   - SetHistoryLength(n): number of frames retained (default 120)
 *   - Reset()
 */

#include <cstdint>
#include <string>
#include <vector>

namespace Engine {

struct ScopeInfo {
    std::string name;
    float lastMs{0};
    float avgMs {0};
    float maxMs {0};
    float budgetMs{0};  ///< 0 = no budget set
};

struct ProfilerBar {
    std::string name;
    float startFrac; ///< [0,1] normalised start in frame budget
    float widthFrac; ///< [0,1] width in frame budget
    bool  overBudget;
};

class ProfilerHUD {
public:
    ProfilerHUD();
    ~ProfilerHUD();

    void Init    (float frameBudgetMs=16.67f);
    void Shutdown();
    void Reset   ();

    // Frame control
    void NewFrame();

    // Scope recording
    void BeginScope(const std::string& name);
    void EndScope  (const std::string& name);

    // Query
    float              GetScopeMs     (const std::string& name) const;
    void               GetScopeHistory(const std::string& name,
                                       float* outMs, uint32_t count) const;
    std::vector<ScopeInfo> GetAllScopes() const;

    float GetFrameMs() const;
    float GetFPS    () const;

    // Budget
    void SetBudgetMs(const std::string& name, float ms);

    // HUD output
    std::vector<ProfilerBar> RenderBars() const;

    // Config
    void SetHistoryLength(uint32_t frames);
    void SetFrameBudgetMs(float ms);

    // Export
    std::string ExportJSON() const;

    // RAII scope guard
    struct ScopeGuard {
        ProfilerHUD& hud;
        std::string  name;
        ScopeGuard(ProfilerHUD& h, const std::string& n) : hud(h), name(n) { hud.BeginScope(name); }
        ~ScopeGuard() { hud.EndScope(name); }
    };

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
