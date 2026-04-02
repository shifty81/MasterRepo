#include "Renderer/Pipeline/ForwardRenderer.h"
#include "Renderer/RHI/RenderDevice.h"
#include "Renderer/RHI/Shader.h"
#include "Renderer/Mesh/MeshRenderer.h"
#include "Renderer/Materials/Material.h"
#include "Core/Logging/Log.h"
#include <vector>

namespace NF {

void ForwardRenderer::Init(RenderDevice* device) {
    m_Device = device;
    NF_LOG_INFO("Renderer", "ForwardRenderer initialised");
}

void ForwardRenderer::Shutdown() {
    m_Queue.clear();
    m_Device = nullptr;
    NF_LOG_INFO("Renderer", "ForwardRenderer shut down");
}

void ForwardRenderer::BeginScene(const Matrix4x4& viewMatrix, const Matrix4x4& projMatrix) {
    m_View = viewMatrix;
    m_Proj = projMatrix;
    m_Queue.clear();
}

void ForwardRenderer::Submit(Mesh& mesh, Material& material, const Matrix4x4& transform) {
    m_Queue.push_back({&mesh, &material, transform});
}

void ForwardRenderer::EndScene() {
    Flush();
}

void ForwardRenderer::Flush() {
    for (auto& cmd : m_Queue) {
        cmd.MaterialPtr->Bind();

        // Upload camera and model matrices to the active shader.
        // The material's Bind() activates its shader, so we can set
        // uniforms immediately after.
        Shader* shader = cmd.MaterialPtr->GetShader();
        if (shader) {
            shader->SetMat4("uView",       m_View);
            shader->SetMat4("uProjection", m_Proj);
            shader->SetMat4("uModel",      cmd.Transform);
        }

        cmd.MeshPtr->Bind();
        cmd.MeshPtr->Draw();
    }
    m_Queue.clear();
}

} // namespace NF
