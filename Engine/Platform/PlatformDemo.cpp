#include "Engine/Platform/Platform.h"
#include <iostream>

int main() {
    std::cout << "CPU cores: " << Engine::Platform::GetCPUCoreCount() << "\n";
    std::cout << "Total memory (MB): " << Engine::Platform::GetTotalMemoryMB() << "\n";
    std::cout << "Working directory: " << Engine::Platform::GetWorkingDirectory() << "\n";
    return 0;
}