/// @file AnimationTests.cpp — Unit tests for the Animation module.
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "Animation/Skeleton/Skeleton.h"
#include "Animation/StateMachine/AnimStateMachine.h"
#include "Animation/BlendTree/BlendNode.h"
#include "Core/Math/Matrix.h"

using namespace NF;
using Catch::Matchers::WithinAbs;

static constexpr float kEps = 1e-5f;

// ---------------------------------------------------------------------------
// Skeleton
// ---------------------------------------------------------------------------

TEST_CASE("Skeleton is empty by default", "[animation][skeleton]") {
    Skeleton skel;
    REQUIRE(skel.BoneCount() == 0);
}

TEST_CASE("Skeleton AddBone increments bone count", "[animation][skeleton]") {
    Skeleton skel;
    skel.AddBone(Bone{"Root", -1, Matrix4x4{}});
    skel.AddBone(Bone{"Spine", 0, Matrix4x4{}});
    REQUIRE(skel.BoneCount() == 2);
}

TEST_CASE("Skeleton GetBoneIndex returns correct index", "[animation][skeleton]") {
    Skeleton skel;
    skel.AddBone(Bone{"Root",  -1, Matrix4x4{}});
    skel.AddBone(Bone{"Spine",  0, Matrix4x4{}});
    skel.AddBone(Bone{"Head",   1, Matrix4x4{}});
    REQUIRE(skel.GetBoneIndex("Root")  == 0);
    REQUIRE(skel.GetBoneIndex("Spine") == 1);
    REQUIRE(skel.GetBoneIndex("Head")  == 2);
}

TEST_CASE("Skeleton GetBoneIndex returns -1 for unknown name", "[animation][skeleton]") {
    Skeleton skel;
    skel.AddBone(Bone{"Root", -1, Matrix4x4{}});
    REQUIRE(skel.GetBoneIndex("Missing") == -1);
}

TEST_CASE("Skeleton GetBone retrieves correct bone data", "[animation][skeleton]") {
    Skeleton skel;
    skel.AddBone(Bone{"Root", -1, Matrix4x4{}});
    const Bone& b = skel.GetBone(0);
    REQUIRE(b.Name == "Root");
    REQUIRE(b.ParentIndex == -1);
}

TEST_CASE("Skeleton GetBone throws on out-of-range index", "[animation][skeleton]") {
    Skeleton skel;
    skel.AddBone(Bone{"Root", -1, Matrix4x4{}});
    REQUIRE_THROWS_AS(skel.GetBone(-1), std::out_of_range);
    REQUIRE_THROWS_AS(skel.GetBone(1),  std::out_of_range);
}

// ---------------------------------------------------------------------------
// ClipNode
// ---------------------------------------------------------------------------

TEST_CASE("ClipNode advances CurrentTime on Evaluate", "[animation][clipnode]") {
    ClipNode node{"Walk", 1.f};
    REQUIRE_THAT(node.CurrentTime, WithinAbs(0.f, kEps));
    Pose pose;
    node.Evaluate(0.5f, pose);
    REQUIRE_THAT(node.CurrentTime, WithinAbs(0.5f, kEps));
    node.Evaluate(0.5f, pose);
    REQUIRE_THAT(node.CurrentTime, WithinAbs(1.f, kEps));
}

TEST_CASE("ClipNode respects PlaybackSpeed", "[animation][clipnode]") {
    ClipNode node{"Run", 2.f}; // double speed
    Pose pose;
    node.Evaluate(1.f, pose);
    REQUIRE_THAT(node.CurrentTime, WithinAbs(2.f, kEps));
}

// ---------------------------------------------------------------------------
// BlendNode2
// ---------------------------------------------------------------------------

TEST_CASE("BlendNode2 with alpha=0 evaluates only NodeA side", "[animation][blendnode]") {
    auto nodeA = std::make_shared<ClipNode>("Idle");
    auto nodeB = std::make_shared<ClipNode>("Walk");

    BlendNode2 blend{nodeA, nodeB, 0.f};
    Pose pose;
    // Should not throw; both children are evaluated for correct blend.
    REQUIRE_NOTHROW(blend.Evaluate(1.f, pose));
    // NodeA time should advance
    REQUIRE_THAT(nodeA->CurrentTime, WithinAbs(1.f, kEps));
}

TEST_CASE("BlendNode2 with null nodes does not crash and leaves pose unchanged", "[animation][blendnode]") {
    BlendNode2 blend{nullptr, nullptr, 0.5f};
    Pose pose;
    pose.BoneTransforms.push_back(Matrix4x4{});
    // Store the original transform to verify it is not modified.
    Matrix4x4 original{};
    blend.Evaluate(1.f, pose);
    // Pose should be unchanged since both child nodes are null.
    REQUIRE(pose.BoneTransforms.size() == 1);
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r)
            REQUIRE(pose.BoneTransforms[0].M[c][r] == original.M[c][r]);
}

TEST_CASE("BlendNode2 alpha is clamped to [0,1]", "[animation][blendnode]") {
    // One bone, identity in both children → blended result must remain identity.
    auto nodeA = std::make_shared<ClipNode>("A");
    auto nodeB = std::make_shared<ClipNode>("B");

    BlendNode2 blend{nodeA, nodeB, 99.f}; // > 1 should clamp to 1
    Pose pose;
    pose.BoneTransforms.push_back(Matrix4x4{});
    REQUIRE_NOTHROW(blend.Evaluate(0.f, pose));
}

// ---------------------------------------------------------------------------
// AnimStateMachine
// ---------------------------------------------------------------------------

TEST_CASE("AnimStateMachine has empty current state by default", "[animation][statemachine]") {
    AnimStateMachine sm;
    REQUIRE(sm.GetCurrentState().empty());
}

TEST_CASE("AnimStateMachine first AddState becomes active state", "[animation][statemachine]") {
    AnimStateMachine sm;
    sm.AddState(AnimState{"Idle", nullptr});
    REQUIRE(sm.GetCurrentState() == "Idle");
}

TEST_CASE("AnimStateMachine transitions when condition is true", "[animation][statemachine]") {
    AnimStateMachine sm;
    sm.AddState(AnimState{"Idle", nullptr});
    sm.AddState(AnimState{"Walk", nullptr});

    bool trigger = false;
    sm.AddTransition(AnimTransition{"Idle", "Walk", [&]{ return trigger; }, 0.f});

    Pose pose;
    sm.Update(0.016f, pose); // condition false, stays in Idle
    REQUIRE(sm.GetCurrentState() == "Idle");

    trigger = true;
    sm.Update(0.016f, pose); // condition true, begins transition
    // With BlendTime = 0, the transition completes immediately on next Update.
    sm.Update(0.f, pose);
    REQUIRE(sm.GetCurrentState() == "Walk");
}

TEST_CASE("AnimStateMachine does not transition when condition is false", "[animation][statemachine]") {
    AnimStateMachine sm;
    sm.AddState(AnimState{"Idle", nullptr});
    sm.AddState(AnimState{"Walk", nullptr});
    sm.AddTransition(AnimTransition{"Idle", "Walk", []{ return false; }, 0.f});

    Pose pose;
    for (int i = 0; i < 10; ++i)
        sm.Update(0.016f, pose);

    REQUIRE(sm.GetCurrentState() == "Idle");
}

TEST_CASE("AnimStateMachine Update evaluates active state blend tree", "[animation][statemachine]") {
    auto clip = std::make_shared<ClipNode>("Walk");
    AnimStateMachine sm;
    sm.AddState(AnimState{"Walk", clip});

    Pose pose;
    sm.Update(1.f, pose);
    REQUIRE_THAT(clip->CurrentTime, WithinAbs(1.f, kEps));
}
