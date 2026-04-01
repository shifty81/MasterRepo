#include "Renderer/PostProcess/PostProcessStack.h"
#include "Renderer/RHI/Texture.h"

namespace NF {

void PostProcessStack::AddPass(std::shared_ptr<PostProcessPass> pass) {
    m_Passes.push_back(std::move(pass));
}

void PostProcessStack::Execute(Texture& scene, Texture& output) {
    if (m_Passes.empty()) return;

    // Single-pass: read directly from scene, write directly to output.
    if (m_Passes.size() == 1) {
        m_Passes[0]->Apply(scene, output);
        return;
    }

    // Multi-pass: ping-pong between two intermediate textures.
    // Pass 0        : scene       -> pingPong[0]
    // Pass 1..N-2   : pingPong[k] -> pingPong[k^1]  (alternating)
    // Pass N-1 (last): pingPong[k] -> output
    Texture pingPong[2];
    pingPong[0].Create(scene.GetWidth(), scene.GetHeight(), TextureFormat::RGBA8);
    pingPong[1].Create(scene.GetWidth(), scene.GetHeight(), TextureFormat::RGBA8);

    m_Passes[0]->Apply(scene, pingPong[0]);

    for (std::size_t i = 1; i + 1 < m_Passes.size(); ++i) {
        Texture& src = pingPong[(i - 1) % 2];
        Texture& dst = pingPong[i       % 2];
        m_Passes[i]->Apply(src, dst);
    }

    m_Passes.back()->Apply(pingPong[(m_Passes.size() - 2) % 2], output);
}

} // namespace NF
