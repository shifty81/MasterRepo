#include "Engine/Scene/SceneGraph.h"
#include <algorithm>
#include <stack>
#include <functional>

namespace Engine {

// ── SceneGraph implementation ─────────────────────────────────────────────────
// A free-function helper library for manipulating a root SceneNode tree.
// All operations work on `SceneNode` values defined in SceneGraph.h.

/// Add a child node to the parent identified by name (breadth-first search).
/// Returns true if the parent was found and the child was inserted.
bool SceneGraph_AddChild(SceneNode& root,
                         const std::string& parentName,
                         SceneNode child)
{
    if (root.name == parentName) {
        root.children.push_back(std::move(child));
        return true;
    }
    for (auto& c : root.children)
        if (SceneGraph_AddChild(c, parentName, child)) return true;
    return false;
}

/// Remove the first child whose name matches (depth-first search).
/// Returns true if a node was removed.
bool SceneGraph_Remove(SceneNode& root, const std::string& name) {
    auto it = std::find_if(root.children.begin(), root.children.end(),
        [&](const SceneNode& n){ return n.name == name; });
    if (it != root.children.end()) {
        root.children.erase(it);
        return true;
    }
    for (auto& c : root.children)
        if (SceneGraph_Remove(c, name)) return true;
    return false;
}

/// Find a node by name (depth-first). Returns nullptr if not found.
SceneNode* SceneGraph_Find(SceneNode& root, const std::string& name) {
    if (root.name == name) return &root;
    for (auto& c : root.children) {
        SceneNode* found = SceneGraph_Find(c, name);
        if (found) return found;
    }
    return nullptr;
}

const SceneNode* SceneGraph_Find(const SceneNode& root,
                                  const std::string& name)
{
    if (root.name == name) return &root;
    for (const auto& c : root.children) {
        const SceneNode* found = SceneGraph_Find(c, name);
        if (found) return found;
    }
    return nullptr;
}

/// Traverse the tree depth-first, invoking visitor for every node.
void SceneGraph_Traverse(SceneNode& root,
                          const std::function<void(SceneNode&, int depth)>& visitor,
                          int depth)
{
    visitor(root, depth);
    for (auto& c : root.children)
        SceneGraph_Traverse(c, visitor, depth + 1);
}

/// Count the total number of nodes (including root).
size_t SceneGraph_Count(const SceneNode& root) {
    size_t count = 1;
    for (const auto& c : root.children) count += SceneGraph_Count(c);
    return count;
}

/// Clear all children of the root node.
void SceneGraph_Clear(SceneNode& root) {
    root.children.clear();
}

} // namespace Engine
