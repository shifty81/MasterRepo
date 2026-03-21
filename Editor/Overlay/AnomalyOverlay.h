#pragma once
#include "Engine/Math/Math.h"
#include <string>
#include <vector>
#include <functional>

namespace Editor {

/// Severity levels for overlay annotations.
enum class OverlaySeverity { Info, Warning, Error };

/// A single overlay annotation drawn in the viewport.
struct OverlayAnnotation {
    std::string      id;           ///< Unique identifier
    std::string      label;        ///< Text shown in the overlay
    Engine::Math::Vec3 worldPos;   ///< 3-D position of the annotation
    OverlaySeverity  severity{OverlaySeverity::Warning};
    std::string      source;       ///< System that raised this (e.g. "PCG", "AI", "Physics")
    bool             visible{true};
};

using AnnotationCallback = std::function<void(const OverlayAnnotation&)>;

/// AnomalyOverlay — visual debug overlay for PCG constraints, AI feedback,
/// and anomaly highlights drawn in the editor viewport.
///
/// Subsystems (PCG, AI, Physics) push OverlayAnnotation objects here.
/// The EditorRenderer queries this manager each frame and draws the
/// appropriate coloured markers, labels, and connecting lines.
class AnomalyOverlay {
public:
    AnomalyOverlay();
    ~AnomalyOverlay();

    // ── visibility ────────────────────────────────────────────
    void SetVisible(bool visible);
    bool IsVisible() const;
    /// Per-source visibility toggle.
    void SetSourceVisible(const std::string& source, bool visible);

    // ── annotation management ─────────────────────────────────
    /// Add or update an annotation.  If an annotation with the same id
    /// already exists it is replaced.
    void Push(const OverlayAnnotation& annotation);
    /// Remove a specific annotation by id.
    bool Remove(const std::string& id);
    /// Remove all annotations from a given source.
    void ClearSource(const std::string& source);
    /// Remove all annotations.
    void ClearAll();

    // ── query ─────────────────────────────────────────────────
    std::vector<OverlayAnnotation> GetAll()    const;
    std::vector<OverlayAnnotation> GetVisible() const;
    size_t Count()                             const;

    // ── callbacks ─────────────────────────────────────────────
    /// Fired when a new annotation is pushed.
    void OnAnnotation(AnnotationCallback cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Editor
