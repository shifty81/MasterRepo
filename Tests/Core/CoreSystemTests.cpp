/// @file CoreSystemTests.cpp — Unit tests for EventBus and TaskSystem.
#include <catch2/catch_test_macros.hpp>
#include "Core/Events/EventBus.h"
#include "Core/Threading/TaskSystem.h"
#include <atomic>
#include <chrono>

using namespace NF;

// ---------------------------------------------------------------------------
// EventBus
// ---------------------------------------------------------------------------

struct TestEvent {
    int Value{0};
};

struct OtherEvent {
    std::string Message;
};

TEST_CASE("EventBus delivers event to a single subscriber", "[core][eventbus]") {
    EventBus bus;
    int received = -1;
    bus.Subscribe<TestEvent>([&](const TestEvent& e) { received = e.Value; });
    bus.Publish(TestEvent{42});
    REQUIRE(received == 42);
}

TEST_CASE("EventBus delivers event to multiple subscribers", "[core][eventbus]") {
    EventBus bus;
    int count = 0;
    bus.Subscribe<TestEvent>([&](const TestEvent&) { ++count; });
    bus.Subscribe<TestEvent>([&](const TestEvent&) { ++count; });
    bus.Subscribe<TestEvent>([&](const TestEvent&) { ++count; });
    bus.Publish(TestEvent{});
    REQUIRE(count == 3);
}

TEST_CASE("EventBus does not call subscribers for other event types", "[core][eventbus]") {
    EventBus bus;
    bool called = false;
    bus.Subscribe<OtherEvent>([&](const OtherEvent&) { called = true; });
    bus.Publish(TestEvent{1});
    REQUIRE_FALSE(called);
}

TEST_CASE("EventBus Publish with no subscribers is a no-op", "[core][eventbus]") {
    EventBus bus;
    REQUIRE_NOTHROW(bus.Publish(TestEvent{99}));
}

TEST_CASE("EventBus Clear removes all handlers", "[core][eventbus]") {
    EventBus bus;
    int count = 0;
    bus.Subscribe<TestEvent>([&](const TestEvent&) { ++count; });
    bus.Clear();
    bus.Publish(TestEvent{});
    REQUIRE(count == 0);
}

TEST_CASE("EventBus passes event data correctly to handler", "[core][eventbus]") {
    EventBus bus;
    std::string got;
    bus.Subscribe<OtherEvent>([&](const OtherEvent& e) { got = e.Message; });
    bus.Publish(OtherEvent{"hello"});
    REQUIRE(got == "hello");
}

TEST_CASE("EventBus supports multiple independent event types simultaneously", "[core][eventbus]") {
    EventBus bus;
    int testCount = 0;
    std::string otherMsg;

    bus.Subscribe<TestEvent>([&](const TestEvent& e) { testCount = e.Value; });
    bus.Subscribe<OtherEvent>([&](const OtherEvent& e) { otherMsg = e.Message; });

    bus.Publish(TestEvent{7});
    bus.Publish(OtherEvent{"world"});

    REQUIRE(testCount == 7);
    REQUIRE(otherMsg == "world");
}

// ---------------------------------------------------------------------------
// TaskSystem
// ---------------------------------------------------------------------------

TEST_CASE("TaskSystem executes a submitted task", "[core][tasksystem]") {
    TaskSystem ts;
    ts.Init(1);

    std::atomic<bool> ran{false};
    auto future = ts.Submit([&] { ran = true; });
    future.get(); // blocks until complete

    REQUIRE(ran.load());
    ts.Shutdown();
}

TEST_CASE("TaskSystem executes multiple tasks", "[core][tasksystem]") {
    TaskSystem ts;
    ts.Init(2);

    std::atomic<int> counter{0};
    constexpr int kTasks = 20;
    std::vector<std::future<void>> futures;
    futures.reserve(kTasks);

    for (int i = 0; i < kTasks; ++i)
        futures.push_back(ts.Submit([&] { ++counter; }));

    for (auto& f : futures)
        f.get();

    REQUIRE(counter.load() == kTasks);
    ts.Shutdown();
}

TEST_CASE("TaskSystem future propagates exceptions", "[core][tasksystem]") {
    TaskSystem ts;
    ts.Init(1);

    auto future = ts.Submit([] { throw std::runtime_error("test"); });
    REQUIRE_THROWS_AS(future.get(), std::runtime_error);

    ts.Shutdown();
}

TEST_CASE("TaskSystem Shutdown with no tasks does not hang", "[core][tasksystem]") {
    TaskSystem ts;
    ts.Init(4);
    REQUIRE_NOTHROW(ts.Shutdown());
}

TEST_CASE("TaskSystem Init with 0 threads uses hardware concurrency", "[core][tasksystem]") {
    TaskSystem ts;
    REQUIRE_NOTHROW(ts.Init(0));

    std::atomic<bool> ran{false};
    ts.Submit([&] { ran = true; }).get();
    REQUIRE(ran.load());
    ts.Shutdown();
}
