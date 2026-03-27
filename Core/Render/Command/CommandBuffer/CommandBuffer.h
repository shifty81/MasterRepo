#pragma once
/**
 * @file CommandBuffer.h
 * @brief Deferred render command recording: push, sort, execute, reset.
 *
 * Features:
 *   - CommandType enum: Draw, DrawIndexed, Dispatch, Clear, Blit, SetPipeline,
 *     SetViewport, SetScissor, PushConstants, Barrier, BeginRenderPass, EndRenderPass
 *   - Push(cmd): append a command; thread-safe variant PushThreadSafe()
 *   - Sort(): sort by key (pass/material/depth)
 *   - Execute(): traverse commands in order and invoke backend dispatch
 *   - Reset(): clear all commands for next frame
 *   - SetSortKey(cmd, key): override per-command 64-bit sort key
 *   - GetCommandCount() → uint32_t
 *   - BeginRecording() / EndRecording()
 *   - Merge(other&): append another buffer's commands
 *   - Reserve(n): pre-allocate n command slots
 */

#include <cstdint>
#include <functional>
#include <vector>

namespace Core {

enum class CommandType : uint8_t {
    Draw, DrawIndexed, Dispatch, Clear, Blit,
    SetPipeline, SetViewport, SetScissor, PushConstants,
    Barrier, BeginRenderPass, EndRenderPass
};

struct RenderCommand {
    CommandType type{CommandType::Draw};
    uint64_t    sortKey{0};
    uint64_t    param[4]{};  // generic payload (handle IDs, counts, offsets…)
};

class CommandBuffer {
public:
    CommandBuffer();
    ~CommandBuffer();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Recording
    void BeginRecording();
    void EndRecording  ();

    // Append
    void Push          (const RenderCommand& cmd);
    void PushThreadSafe(const RenderCommand& cmd);

    // Merge
    void Merge(const CommandBuffer& other);

    // Sort by sortKey ascending
    void Sort();

    // Execute: calls backend dispatch for each command in order
    void Execute(std::function<void(const RenderCommand&)> backend);

    // Config
    void Reserve(uint32_t n);
    void SetSortKey(uint32_t index, uint64_t key);

    // Query
    uint32_t         GetCommandCount() const;
    const RenderCommand& GetCommand (uint32_t index) const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Core
