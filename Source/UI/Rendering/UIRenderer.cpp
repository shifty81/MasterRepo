#include "Source/UI/Rendering/UIRenderer.h"
#include "Source/Core/Logging/Log.h"

namespace NF {

bool UIRenderer::Init() {
    m_Initialised = true;
    NF_LOG_INFO("UI", "UIRenderer initialised");
    return true;
}

void UIRenderer::Shutdown() {
    m_Initialised = false;
    NF_LOG_INFO("UI", "UIRenderer shut down");
}

void UIRenderer::BeginFrame() {
    // Per-frame setup (e.g. reset draw lists, bind UI projection matrix).
}

void UIRenderer::DrawRect(const Rect& rect, uint32_t color) {
    // Batch the quad into the current draw list.
    (void)rect; (void)color;
}

void UIRenderer::DrawText(std::string_view text, float x, float y, uint32_t color) {
    // Batch glyph quads from the font atlas into the current draw list.
    (void)text; (void)x; (void)y; (void)color;
}

void UIRenderer::EndFrame() {
    // Flush batched draw calls to the GPU.
}

} // namespace NF
