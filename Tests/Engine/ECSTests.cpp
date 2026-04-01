/// @file ECSTests.cpp — Unit tests for the Entity-Component System.
#include <catch2/catch_test_macros.hpp>
#include "Engine/ECS/World.h"

using namespace NF;

struct Position {
    float X{0.f}, Y{0.f};
};

struct Velocity {
    float VX{0.f}, VY{0.f};
};

struct Health {
    int Points{100};
};

TEST_CASE("World creates entities with unique IDs", "[ecs][world]") {
    World world;
    const EntityId e1 = world.CreateEntity();
    const EntityId e2 = world.CreateEntity();
    const EntityId e3 = world.CreateEntity();

    REQUIRE(e1 != e2);
    REQUIRE(e2 != e3);
    REQUIRE(e1 != e3);
}

TEST_CASE("AddComponent and GetComponent round-trip", "[ecs][world]") {
    World world;
    const EntityId e = world.CreateEntity();

    world.AddComponent<Position>(e, Position{3.f, 7.f});
    REQUIRE(world.HasComponent<Position>(e));

    const Position& pos = world.GetComponent<Position>(e);
    REQUIRE(pos.X == 3.f);
    REQUIRE(pos.Y == 7.f);
}

TEST_CASE("HasComponent returns false for missing components", "[ecs][world]") {
    World world;
    const EntityId e = world.CreateEntity();
    REQUIRE_FALSE(world.HasComponent<Velocity>(e));
}

TEST_CASE("HasComponent returns false for missing component type", "[ecs][world]") {
    World world;
    const EntityId e = world.CreateEntity();
    REQUIRE_FALSE(world.HasComponent<Health>(e));
}

TEST_CASE("Multiple components on the same entity", "[ecs][world]") {
    World world;
    const EntityId e = world.CreateEntity();

    world.AddComponent<Position>(e, Position{1.f, 2.f});
    world.AddComponent<Velocity>(e, Velocity{3.f, 4.f});
    world.AddComponent<Health>(e,   Health{50});

    REQUIRE(world.HasComponent<Position>(e));
    REQUIRE(world.HasComponent<Velocity>(e));
    REQUIRE(world.HasComponent<Health>(e));

    REQUIRE(world.GetComponent<Velocity>(e).VX == 3.f);
    REQUIRE(world.GetComponent<Health>(e).Points == 50);
}

TEST_CASE("DestroyEntity removes the entity and its components", "[ecs][world]") {
    World world;
    const EntityId e = world.CreateEntity();
    world.AddComponent<Position>(e, Position{9.f, 9.f});

    world.DestroyEntity(e);

    // After destruction the component should no longer be present
    REQUIRE_FALSE(world.HasComponent<Position>(e));
}

TEST_CASE("View iterates only entities with the requested component", "[ecs][world]") {
    World world;
    const EntityId e1 = world.CreateEntity();
    const EntityId e2 = world.CreateEntity();
    const EntityId e3 = world.CreateEntity();

    world.AddComponent<Position>(e1, Position{1.f, 0.f});
    world.AddComponent<Position>(e3, Position{3.f, 0.f});
    // e2 has no Position

    int count = 0;
    for (const auto& [id, pos] : world.View<Position>()) {
        (void)pos;
        REQUIRE((id == e1 || id == e3));
        ++count;
    }
    REQUIRE(count == 2);
}
