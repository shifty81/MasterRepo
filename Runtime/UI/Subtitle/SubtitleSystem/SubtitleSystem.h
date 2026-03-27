#pragma once
/**
 * @file SubtitleSystem.h
 * @brief Caption/subtitle display: timed entries, queue, line-wrap, localisation hook.
 *
 * Features:
 *   - SubtitleEntry: text, start time, duration, speaker, style
 *   - Queue: ordered by start time, active set at current media time
 *   - Line-wrap: max chars per line, split on word boundary
 *   - Localisation hook: TranslateFn called per entry at queue time
 *   - Speaker colour map: speaker name → colour
 *   - Fade-in/fade-out alpha per entry
 *   - Multiple simultaneous entries (e.g. dual-language)
 *   - Media time: driven by Tick(dt) or SetTime(t)
 *   - GetActive() → entries currently visible
 *   - On-show / on-hide callbacks
 *   - Clear, Pause, Resume
 *   - SRT import (text format)
 *
 * Typical usage:
 * @code
 *   SubtitleSystem ss;
 *   ss.Init(50);
 *   ss.AddEntry({"Hello, world!", 1.5f, 3.f, "Player"});
 *   ss.SetMaxLineWidth(40);
 *   ss.Tick(dt);
 *   for (auto& e : ss.GetActive()) Render(e.text, e.alpha);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

struct SubtitleStyle {
    float colour[4]{1,1,1,1};
    float fontSize{1.f};
    bool  bold    {false};
    bool  italic  {false};
};

struct SubtitleEntry {
    uint32_t      id{0};
    std::string   text;
    float         startTime{0.f};
    float         duration {3.f};
    std::string   speaker;
    SubtitleStyle style;
    float         fadeIn  {0.15f};
    float         fadeOut {0.3f};
    // Runtime
    float         alpha   {0.f};
    bool          active  {false};
    std::vector<std::string> wrappedLines;
};

class SubtitleSystem {
public:
    SubtitleSystem();
    ~SubtitleSystem();

    void Init    (uint32_t maxEntries=100);
    void Shutdown();
    void Tick    (float dt);
    void SetTime (float t);                ///< jump to time
    void Pause   (bool paused);
    void Clear   ();

    // Entry management
    uint32_t AddEntry   (const SubtitleEntry& e);
    void     RemoveEntry(uint32_t id);
    bool     HasEntry   (uint32_t id) const;

    // Line wrapping
    void SetMaxLineWidth(uint32_t chars);

    // Speaker colours
    void SetSpeakerColour(const std::string& speaker, const float rgba[4]);

    // Localisation
    void SetTranslateFn(std::function<std::string(const std::string&)> fn);

    // Active entries
    std::vector<const SubtitleEntry*> GetActive() const;
    float CurrentTime() const;
    bool  IsPaused()    const;

    // SRT import
    bool LoadSRT(const std::string& path);

    // Callbacks
    void SetOnShow(std::function<void(const SubtitleEntry&)> cb);
    void SetOnHide(std::function<void(uint32_t id)> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
