#pragma once
/**
 * @file SceneGraph.h
 * @brief Entity hierarchy with local/world transform propagation, dirty tracking, and traversal.
 *
 * Features:
 *   - CreateNode(name) → nodeId
 *   - DestroyNode(id): recursively destroys children
 *   - SetParent(childId, parentId): reparent a node (0 = root)
 *   - SetLocalTransform(id, pos, rot[4], scale): set node's local TRS
 *   - GetLocalTransform / GetWorldTransform: accessors
 *   - GetWorldMatrix(id) → Mat4x4: combined world-space 4×4 matrix
 *   - Dirty propagation: marking a node dirty marks all descendants dirty
 *   - Update(): recomputes world matrices depth-first for all dirty nodes
 *   - GetChildren(id) → ids[]: direct child list
 *   - GetDescendants(id) → ids[]: full subtree (depth-first)
 *   - FindByName(name) → id or 0
 *   - AttachComponent(nodeId, componentTypeId, ptr): tag arbitrary component on node
 *   - GetComponent(nodeId, typeId) → void*
 *   - Traverse(visitor): depth-first visitor callback
 *   - GetNodeCount() → uint32_t
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

struct SGVec3 { float x, y, z; };
struct SGQuat { float x, y, z, w; };
struct SGMat4 { float m[16]; };

struct SGTransform {
    SGVec3 pos  {0,0,0};
    SGQuat rot  {0,0,0,1};
    SGVec3 scale{1,1,1};
};

class SceneGraph {
public:
    SceneGraph();
    ~SceneGraph();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Node management
    uint32_t CreateNode (const std::string& name, uint32_t parentId=0);
    void     DestroyNode(uint32_t id);
    bool     Exists     (uint32_t id) const;

    // Hierarchy
    void                    SetParent   (uint32_t childId, uint32_t parentId);
    uint32_t                GetParent   (uint32_t id) const;
    std::vector<uint32_t>   GetChildren (uint32_t id) const;
    std::vector<uint32_t>   GetDescendants(uint32_t id) const;

    // Transforms
    void           SetLocalTransform(uint32_t id, const SGTransform& t);
    SGTransform    GetLocalTransform (uint32_t id) const;
    SGTransform    GetWorldTransform (uint32_t id) const;
    SGMat4         GetWorldMatrix    (uint32_t id) const;

    // Update (recompute dirty world matrices)
    void Update();

    // Component attachment (opaque pointer, caller manages lifetime)
    void  AttachComponent(uint32_t nodeId, uint32_t typeId, void* ptr);
    void* GetComponent   (uint32_t nodeId, uint32_t typeId) const;

    // Search
    uint32_t FindByName(const std::string& name) const;
    std::string GetName(uint32_t id) const;

    // Traversal
    void Traverse(std::function<void(uint32_t id, uint32_t depth)> visitor) const;

    uint32_t GetNodeCount() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
