#pragma once
/**
 * @file DebugOverlay.h
 * @brief Frame-persistent debug annotation layer: text, lines, rects, graphs, watches.
 *
 * Features:
 *   - DrawText(x, y, text, colour, duration): add screen-space text annotation
 *   - DrawLine(x0,y0,x1,y1, colour, duration): add screen-space line
 *   - DrawRect(x,y,w,h, colour, filled, duration)
 *   - DrawCircle(cx,cy,r, colour, segments, duration)
 *   - DrawBar(x,y,w,h, value, maxValue, colour, label, duration): horizontal fill bar
 *   - AddWatch(name, valueFn): register a per-frame value watch (auto-displays in watch panel)
 *   - RemoveWatch(name)
 *   - AddGraph(name, historyLen): creates a rolling graph channel
 *   - PushGraphValue(name, value): push a new sample to named graph
 *   - DrawGraphPanel(x, y, w, h): render all active graphs in a panel
 *   - Clear(): remove all timed primitives (watches/graphs persist)
 *   - Tick(dt): advance durations, expire timed primitives
 *   - Render(cb): iterate all active primitives, calls cb(primitive) for external draw
 *   - SetVisible(on) / IsVisible() → bool
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

struct OverlayColor { float r,g,b,a; };

enum class OverlayPrimType : uint8_t {
    Text, Line, Rect, Circle, Bar, Graph
};

struct OverlayPrimitive {
    OverlayPrimType type;
    float           x,y,x1,y1;
    float           w,h,r;       ///< width, height, radius
    float           value,maxVal;
    uint32_t        segments;
    bool            filled;
    OverlayColor    colour;
    std::string     text;
    std::string     label;
    float           duration;    ///< remaining lifetime (-1 = permanent)
};

class DebugOverlay {
public:
    DebugOverlay();
    ~DebugOverlay();

    void Init    ();
    void Shutdown();
    void Reset   ();
    void Tick    (float dt);

    // Primitives
    void DrawText  (float x, float y, const std::string& text,
                    OverlayColor col={1,1,1,1}, float duration=0.f);
    void DrawLine  (float x0, float y0, float x1, float y1,
                    OverlayColor col={1,1,0,1}, float duration=0.f);
    void DrawRect  (float x, float y, float w, float h,
                    OverlayColor col={1,0,0,1}, bool filled=false, float duration=0.f);
    void DrawCircle(float cx, float cy, float r,
                    OverlayColor col={0,1,1,1}, uint32_t segs=16, float duration=0.f);
    void DrawBar   (float x, float y, float w, float h,
                    float value, float maxValue,
                    OverlayColor col={0,1,0,1}, const std::string& label="",
                    float duration=0.f);

    // Watches
    void AddWatch   (const std::string& name, std::function<std::string()> valueFn);
    void RemoveWatch(const std::string& name);

    // Graphs
    void AddGraph      (const std::string& name, uint32_t historyLen=120);
    void PushGraphValue(const std::string& name, float value);
    void DrawGraphPanel(float x, float y, float w, float h);

    // Control
    void Clear     ();
    void SetVisible(bool on);
    bool IsVisible () const;

    // Rendering callback
    void Render(std::function<void(const OverlayPrimitive&)> drawCb) const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
