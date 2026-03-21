#include <iostream>
#include "Engine/Math/Math.h"

int main() {
    using namespace Engine::Math;
    Vec2 a{1, 2}, b{3, 4};
    Vec2 c = a + b;
    std::cout << "Vec2 c = (" << c.x << ", " << c.y << ")\n";
    Vec3 v1{1, 0, 0}, v2{0, 1, 0};
    Vec3 cross = v1.Cross(v2);
    std::cout << "Vec3 cross = (" << cross.x << ", " << cross.y << ", " << cross.z << ")\n";
    return 0;
}