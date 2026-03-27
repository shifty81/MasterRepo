#pragma once
/**
 * @file RenderGraph.h
 * @brief Directed-acyclic render-pass graph with automatic resource aliasing and barrier insertion.
 *
 * Features:
 *   - AddPass(name, setupFn, executeFn) → passId: register a named render pass
 *   - AddResource(name, desc) → resourceId: declare a transient texture/buffer
 *   - DeclareRead(passId, resourceId) / DeclareWrite(passId, resourceId): data-flow edges
 *   - Compile(): topological sort, lifetime analysis, resource aliasing
 *   - Execute(): call execute callbacks in compiled order, emit resource barriers
 *   - GetPassOrder() → passIds[]: sorted execution list
 *   - GetResourceLifetime(id) → (firstPass, lastPass): begin/end usage range
 *   - AddDependency(afterPass, beforePass): explicit ordering edge
 *   - SetOnBarrier(cb): callback for each synthetic barrier event
 *   - Reset() / Shutdown()
 *   - GetPassCount() / GetResourceCount()
 *   - IsCompiled() → bool
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

struct RGResourceDesc {
    uint32_t width  {1};
    uint32_t height {1};
    uint32_t depth  {1};
    uint32_t mips   {1};
    uint32_t format {0}; ///< opaque format token
    bool     isBuffer{false};
};

struct RGBarrierEvent {
    uint32_t    resourceId;
    uint32_t    srcPassId;
    uint32_t    dstPassId;
    bool        toWrite; ///< true = transition to write state
};

class RenderGraph {
public:
    RenderGraph();
    ~RenderGraph();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Resource declaration
    uint32_t AddResource(const std::string& name, const RGResourceDesc& desc);
    uint32_t GetResourceId(const std::string& name) const;

    // Pass authoring
    uint32_t AddPass(const std::string& name,
                     std::function<void(uint32_t passId)> setupFn,
                     std::function<void(uint32_t passId)> executeFn);
    void DeclareRead (uint32_t passId, uint32_t resourceId);
    void DeclareWrite(uint32_t passId, uint32_t resourceId);
    void AddDependency(uint32_t afterPassId, uint32_t beforePassId);

    // Compilation & execution
    bool Compile();
    void Execute();

    // Query
    std::vector<uint32_t> GetPassOrder() const;
    void GetResourceLifetime(uint32_t resourceId,
                             uint32_t& outFirstPass, uint32_t& outLastPass) const;
    uint32_t GetPassCount    () const;
    uint32_t GetResourceCount() const;
    bool     IsCompiled      () const;

    // Barrier callback
    void SetOnBarrier(std::function<void(const RGBarrierEvent&)> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
