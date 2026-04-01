/// @file PhysicsTests.cpp — Unit tests for the Physics module.
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "Physics/Dynamics/RigidBody.h"
#include "Physics/Dynamics/PhysicsWorld.h"
#include "Physics/Collision/CollisionDetection.h"

using namespace NF;
using Catch::Matchers::WithinAbs;

static constexpr float kEps = 1e-4f;

// ---------------------------------------------------------------------------
// RigidBody
// ---------------------------------------------------------------------------

TEST_CASE("RigidBody default state", "[physics][rigidbody]") {
    RigidBody rb;
    REQUIRE_THAT(rb.GetState().Position.X,  WithinAbs(0.f, kEps));
    REQUIRE_THAT(rb.GetState().Velocity.X,  WithinAbs(0.f, kEps));
    REQUIRE_THAT(rb.GetState().Mass,        WithinAbs(1.f, kEps));
    REQUIRE_FALSE(rb.GetState().IsKinematic);
}

TEST_CASE("RigidBody Integrate moves position", "[physics][rigidbody]") {
    RigidBodyState s;
    s.Velocity = {2.f, 0.f, 0.f};
    RigidBody rb{s};
    rb.Integrate(1.f);
    REQUIRE_THAT(rb.GetState().Position.X, WithinAbs(2.f, kEps));
    REQUIRE_THAT(rb.GetState().Position.Y, WithinAbs(0.f, kEps));
}

TEST_CASE("RigidBody ApplyForce changes velocity via integration", "[physics][rigidbody]") {
    RigidBodyState s;
    s.Mass = 2.f;
    RigidBody rb{s};
    rb.ApplyForce({10.f, 0.f, 0.f}); // F = 10 N, a = 5 m/s²
    rb.Integrate(1.f);
    REQUIRE_THAT(rb.GetState().Velocity.X, WithinAbs(5.f, kEps));
}

TEST_CASE("RigidBody accumulated force resets after Integrate", "[physics][rigidbody]") {
    RigidBody rb;
    rb.ApplyForce({10.f, 0.f, 0.f});
    rb.Integrate(1.f);
    rb.Integrate(1.f); // no force applied this frame
    // Only 1 second of acceleration (1st frame), then constant velocity (2nd frame)
    REQUIRE_THAT(rb.GetState().Velocity.X, WithinAbs(10.f, kEps));
    REQUIRE_THAT(rb.GetState().Position.X, WithinAbs(10.f + 10.f, kEps)); // 10 + 10*1
}

TEST_CASE("RigidBody ApplyImpulse changes velocity immediately", "[physics][rigidbody]") {
    RigidBodyState s;
    s.Mass = 2.f;
    RigidBody rb{s};
    rb.ApplyImpulse({6.f, 0.f, 0.f}); // impulse / mass = 3 m/s
    REQUIRE_THAT(rb.GetState().Velocity.X, WithinAbs(3.f, kEps));
}

TEST_CASE("RigidBody kinematic body ignores forces and impulses", "[physics][rigidbody]") {
    RigidBodyState s;
    s.IsKinematic = true;
    RigidBody rb{s};
    rb.ApplyForce({100.f, 0.f, 0.f});
    rb.ApplyImpulse({100.f, 0.f, 0.f});
    rb.Integrate(1.f);
    REQUIRE_THAT(rb.GetState().Velocity.X, WithinAbs(0.f, kEps));
    REQUIRE_THAT(rb.GetState().Position.X, WithinAbs(0.f, kEps));
}

TEST_CASE("RigidBody zero dt does not move body", "[physics][rigidbody]") {
    RigidBodyState s;
    s.Velocity = {5.f, 0.f, 0.f};
    RigidBody rb{s};
    rb.Integrate(0.f);
    REQUIRE_THAT(rb.GetState().Position.X, WithinAbs(0.f, kEps));
}

// ---------------------------------------------------------------------------
// CollisionDetection — AABB
// ---------------------------------------------------------------------------

TEST_CASE("AABB overlap when touching faces", "[physics][collision]") {
    const AABB a{{0.f, 0.f, 0.f}, {1.f, 1.f, 1.f}};
    const AABB b{{1.f, 0.f, 0.f}, {2.f, 1.f, 1.f}};
    REQUIRE(CollisionDetection::TestAABBAABB(a, b));
}

TEST_CASE("AABB no overlap when separated on X", "[physics][collision]") {
    const AABB a{{0.f, 0.f, 0.f}, {1.f, 1.f, 1.f}};
    const AABB b{{2.f, 0.f, 0.f}, {3.f, 1.f, 1.f}};
    REQUIRE_FALSE(CollisionDetection::TestAABBAABB(a, b));
}

TEST_CASE("AABB overlap when one contains the other", "[physics][collision]") {
    const AABB outer{{-2.f, -2.f, -2.f}, {2.f, 2.f, 2.f}};
    const AABB inner{{-1.f, -1.f, -1.f}, {1.f, 1.f, 1.f}};
    REQUIRE(CollisionDetection::TestAABBAABB(outer, inner));
    REQUIRE(CollisionDetection::TestAABBAABB(inner, outer));
}

// ---------------------------------------------------------------------------
// CollisionDetection — Sphere
// ---------------------------------------------------------------------------

TEST_CASE("Spheres overlap when distance < sum of radii", "[physics][collision]") {
    const Sphere a{Vector3{0.f, 0.f, 0.f}, 1.f};
    const Sphere b{Vector3{1.5f, 0.f, 0.f}, 1.f};
    REQUIRE(CollisionDetection::TestSphereSphere(a, b));
}

TEST_CASE("Spheres do not overlap when distance > sum of radii", "[physics][collision]") {
    const Sphere a{Vector3{0.f, 0.f, 0.f}, 0.5f};
    const Sphere b{Vector3{3.f, 0.f, 0.f}, 0.5f};
    REQUIRE_FALSE(CollisionDetection::TestSphereSphere(a, b));
}

TEST_CASE("Spheres overlap exactly at tangent point", "[physics][collision]") {
    const Sphere a{Vector3{0.f, 0.f, 0.f}, 1.f};
    const Sphere b{Vector3{2.f, 0.f, 0.f}, 1.f};
    REQUIRE(CollisionDetection::TestSphereSphere(a, b));
}

// ---------------------------------------------------------------------------
// CollisionDetection — Raycast
// ---------------------------------------------------------------------------

TEST_CASE("Raycast hits AABB from outside along +X", "[physics][raycast]") {
    const AABB box{{-0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}};
    RaycastHit hit{};
    const bool result = CollisionDetection::Raycast(
        {-2.f, 0.f, 0.f}, {1.f, 0.f, 0.f}, 10.f, box, hit);
    REQUIRE(result);
    REQUIRE_THAT(hit.Distance, WithinAbs(1.5f, kEps));
}

TEST_CASE("Raycast misses AABB aimed away", "[physics][raycast]") {
    const AABB box{{-0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}};
    RaycastHit hit{};
    const bool result = CollisionDetection::Raycast(
        {-2.f, 0.f, 0.f}, {-1.f, 0.f, 0.f}, 10.f, box, hit);
    REQUIRE_FALSE(result);
}

TEST_CASE("Raycast respects maxDist and misses a distant box", "[physics][raycast]") {
    const AABB box{{9.f, -0.5f, -0.5f}, {10.f, 0.5f, 0.5f}};
    RaycastHit hit{};
    const bool result = CollisionDetection::Raycast(
        {0.f, 0.f, 0.f}, {1.f, 0.f, 0.f}, 5.f, box, hit);
    REQUIRE_FALSE(result);
}

// ---------------------------------------------------------------------------
// PhysicsWorld
// ---------------------------------------------------------------------------

TEST_CASE("PhysicsWorld Step integrates all bodies", "[physics][world]") {
    RigidBodyState s;
    s.Velocity = {1.f, 0.f, 0.f};
    RigidBody rb{s};

    PhysicsWorld world;
    world.AddBody(&rb);
    world.Step(2.f);

    REQUIRE_THAT(rb.GetState().Position.X, WithinAbs(2.f, kEps));
}

TEST_CASE("PhysicsWorld RemoveBody excludes body from Step", "[physics][world]") {
    RigidBodyState s;
    s.Velocity = {1.f, 0.f, 0.f};
    RigidBody rb{s};

    PhysicsWorld world;
    world.AddBody(&rb);
    world.RemoveBody(&rb);
    world.Step(1.f);

    REQUIRE_THAT(rb.GetState().Position.X, WithinAbs(0.f, kEps));
}

TEST_CASE("PhysicsWorld AddBody ignores duplicates", "[physics][world]") {
    RigidBodyState s;
    s.Velocity = {1.f, 0.f, 0.f};
    RigidBody rb{s};

    PhysicsWorld world;
    world.AddBody(&rb);
    world.AddBody(&rb); // second add should be ignored
    world.Step(1.f);

    // If the body were added twice, it would be integrated twice per Step.
    REQUIRE_THAT(rb.GetState().Position.X, WithinAbs(1.f, kEps));
}

TEST_CASE("PhysicsWorld Raycast hits nearest body", "[physics][world]") {
    RigidBodyState s1, s2;
    s1.Position = {3.f, 0.f, 0.f};
    s2.Position = {6.f, 0.f, 0.f};
    RigidBody rb1{s1}, rb2{s2};

    PhysicsWorld world;
    world.AddBody(&rb1);
    world.AddBody(&rb2);

    RaycastHit hit{};
    const bool result = world.Raycast({0.f, 0.f, 0.f}, {1.f, 0.f, 0.f}, 20.f, hit);
    REQUIRE(result);
    // rb1 is closer; its AABB spans [2.5, 3.5] so tMin ≈ 2.5
    REQUIRE_THAT(hit.Distance, WithinAbs(2.5f, kEps));
}

TEST_CASE("PhysicsWorld Raycast returns false when no body is hit", "[physics][world]") {
    RigidBodyState s;
    s.Position = {0.f, 5.f, 0.f};
    RigidBody rb{s};

    PhysicsWorld world;
    world.AddBody(&rb);

    RaycastHit hit{};
    // Ray travels along +X, body is displaced along +Y — should miss
    const bool result = world.Raycast({0.f, 0.f, 0.f}, {1.f, 0.f, 0.f}, 20.f, hit);
    REQUIRE_FALSE(result);
}
