#include "Editor/Overlay/AnomalyOverlay.h"
#include <algorithm>
#include <unordered_map>

namespace Editor {

struct AnomalyOverlay::Impl {
    bool                       globalVisible{true};
    std::unordered_map<std::string, bool> sourceVisible;
    std::vector<OverlayAnnotation>        annotations;
    std::vector<AnnotationCallback>       callbacks;
};

AnomalyOverlay::AnomalyOverlay() : m_impl(new Impl()) {}
AnomalyOverlay::~AnomalyOverlay() { delete m_impl; }

void AnomalyOverlay::SetVisible(bool v) { m_impl->globalVisible = v; }
bool AnomalyOverlay::IsVisible()  const { return m_impl->globalVisible; }

void AnomalyOverlay::SetSourceVisible(const std::string& source, bool v) {
    m_impl->sourceVisible[source] = v;
}

void AnomalyOverlay::Push(const OverlayAnnotation& annotation) {
    auto it = std::find_if(m_impl->annotations.begin(),
                           m_impl->annotations.end(),
        [&](const OverlayAnnotation& a){ return a.id == annotation.id; });
    if (it != m_impl->annotations.end())
        *it = annotation;
    else
        m_impl->annotations.push_back(annotation);
    for (auto& cb : m_impl->callbacks) cb(annotation);
}

bool AnomalyOverlay::Remove(const std::string& id) {
    auto it = std::find_if(m_impl->annotations.begin(),
                           m_impl->annotations.end(),
        [&](const OverlayAnnotation& a){ return a.id == id; });
    if (it == m_impl->annotations.end()) return false;
    m_impl->annotations.erase(it);
    return true;
}

void AnomalyOverlay::ClearSource(const std::string& source) {
    m_impl->annotations.erase(
        std::remove_if(m_impl->annotations.begin(),
                       m_impl->annotations.end(),
            [&](const OverlayAnnotation& a){ return a.source == source; }),
        m_impl->annotations.end());
}

void AnomalyOverlay::ClearAll() { m_impl->annotations.clear(); }

std::vector<OverlayAnnotation> AnomalyOverlay::GetAll() const {
    return m_impl->annotations;
}

std::vector<OverlayAnnotation> AnomalyOverlay::GetVisible() const {
    if (!m_impl->globalVisible) return {};
    std::vector<OverlayAnnotation> out;
    for (auto& a : m_impl->annotations) {
        if (!a.visible) continue;
        auto sit = m_impl->sourceVisible.find(a.source);
        if (sit != m_impl->sourceVisible.end() && !sit->second) continue;
        out.push_back(a);
    }
    return out;
}

size_t AnomalyOverlay::Count() const { return m_impl->annotations.size(); }

void AnomalyOverlay::OnAnnotation(AnnotationCallback cb) {
    m_impl->callbacks.push_back(std::move(cb));
}

} // namespace Editor
