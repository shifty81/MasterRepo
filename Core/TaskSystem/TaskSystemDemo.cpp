#include <iostream>
#include <chrono>
#include "Core/TaskSystem/TaskSystem.h"

int main() {
    using namespace Core::TaskSystem;
    TaskSystem ts(2); // 2 worker threads

    auto id1 = ts.Submit({
        .Work = [] { std::cout << "Task 1 running\n"; std::this_thread::sleep_for(std::chrono::milliseconds(100)); },
        .Priority = TaskPriority::Normal,
        .Name = "Task1"
    });
    auto id2 = ts.Submit({
        .Work = [] { std::cout << "Task 2 running\n"; std::this_thread::sleep_for(std::chrono::milliseconds(50)); },
        .Priority = TaskPriority::High,
        .Name = "Task2"
    });
    auto id3 = ts.Submit({
        .Work = [] { std::cout << "Task 3 running\n"; },
        .Priority = TaskPriority::Low,
        .Name = "Task3"
    });

    ts.WaitForAll();
    ts.Shutdown();
    std::cout << "All tasks complete." << std::endl;
    return 0;
}
