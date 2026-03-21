#include "Engine/Scene/SceneGraph.h"
#include <iostream>

int main() {
    Engine::SceneNode root{"Root"};
    root.children.push_back({"Child1", {1,2,3}});
    root.children.push_back({"Child2", {4,5,6}});
    Engine::Camera cam;
    std::cout << "Camera at (" << cam.position.x << ", " << cam.position.y << ", " << cam.position.z << ")\n";
    std::cout << "Scene root: " << root.name << ", children: " << root.children.size() << std::endl;
    for (const auto& child : root.children) {
        std::cout << "  Child: " << child.name << " at (" << child.position.x << ", " << child.position.y << ", " << child.position.z << ")\n";
    }
    return 0;
}
