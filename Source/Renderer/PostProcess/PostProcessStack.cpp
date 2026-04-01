#include "Source/Renderer/PostProcess/PostProcessStack.h"
#include "Source/Renderer/RHI/Texture.h"

namespace NF {

void PostProcessStack::AddPass(std::shared_ptr<PostProcessPass> pass) {
    m_Passes.push_back(std::move(pass));
}

void PostProcessStack::Execute(Texture& scene, Texture& output) {
    if (m_Passes.empty()) return;

    if (m_Passes.size() == 1) {
        m_Passes[0]->Apply(scene, output);
        return;
    }

    // Ping-pong between two intermediate textures for multi-pass chains.
    Texture pingPong[2];
    pingPong[0].Create(scene.GetWidth(), scene.GetHeight(), TextureFormat::RGBA8);
    pingPong[1].Create(scene.GetWidth(), scene.GetHeight(), TextureFormat::RGBA8);

    // First pass reads from scene, writes to pingPong[0].
    m_Passes[0]->Apply(scene, pingPong[0]);

    for (std::size_t i = 1; i + 1 < m_Passes.size(); ++i) {
        Texture& src = pingPong[(i - 1) % 2];
        Texture& dst = pingPong[i       % 2];
        m_Passes[i]->Apply(src, dst);
    }

    // Final pass writes to the caller-supplied output.
    m_Passes.back()->Apply(pingPong[(m_Passes.size() - 2) % 2], output);
}

} // namespace NF
