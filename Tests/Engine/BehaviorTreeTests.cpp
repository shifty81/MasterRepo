/// @file BehaviorTreeTests.cpp — Unit tests for the AI Behavior Tree.
#include <catch2/catch_test_macros.hpp>
#include "Engine/AI/BehaviorTree.h"
#include <vector>

using namespace NF;

static std::shared_ptr<BTLeaf> MakeLeaf(NodeStatus result) {
    return std::make_shared<BTLeaf>([result](float) { return result; });
}

static std::shared_ptr<BTLeaf> MakeCountingLeaf(std::vector<int>& ticks, NodeStatus result) {
    return std::make_shared<BTLeaf>([&ticks, result](float) {
        ticks.push_back(1);
        return result;
    });
}

TEST_CASE("BTLeaf returns configured status", "[ai][bt]") {
    REQUIRE(MakeLeaf(NodeStatus::Success)->Tick(0.f) == NodeStatus::Success);
    REQUIRE(MakeLeaf(NodeStatus::Failure)->Tick(0.f) == NodeStatus::Failure);
    REQUIRE(MakeLeaf(NodeStatus::Running)->Tick(0.f) == NodeStatus::Running);
}

TEST_CASE("BTSequence succeeds when all children succeed", "[ai][bt]") {
    auto seq = std::make_shared<BTSequence>();
    seq->AddChild(MakeLeaf(NodeStatus::Success));
    seq->AddChild(MakeLeaf(NodeStatus::Success));
    REQUIRE(seq->Tick(0.f) == NodeStatus::Success);
}

TEST_CASE("BTSequence fails on first failing child", "[ai][bt]") {
    std::vector<int> ticks;
    auto seq = std::make_shared<BTSequence>();
    seq->AddChild(MakeLeaf(NodeStatus::Failure));
    seq->AddChild(MakeCountingLeaf(ticks, NodeStatus::Success)); // must not run

    REQUIRE(seq->Tick(0.f) == NodeStatus::Failure);
    REQUIRE(ticks.empty());
}

TEST_CASE("BTSequence returns Running when a child returns Running", "[ai][bt]") {
    auto seq = std::make_shared<BTSequence>();
    seq->AddChild(MakeLeaf(NodeStatus::Success));
    seq->AddChild(MakeLeaf(NodeStatus::Running));
    REQUIRE(seq->Tick(0.f) == NodeStatus::Running);
}

TEST_CASE("BTSelector succeeds on first successful child", "[ai][bt]") {
    std::vector<int> ticks;
    auto sel = std::make_shared<BTSelector>();
    sel->AddChild(MakeLeaf(NodeStatus::Success));
    sel->AddChild(MakeCountingLeaf(ticks, NodeStatus::Success)); // must not run

    REQUIRE(sel->Tick(0.f) == NodeStatus::Success);
    REQUIRE(ticks.empty());
}

TEST_CASE("BTSelector fails when all children fail", "[ai][bt]") {
    auto sel = std::make_shared<BTSelector>();
    sel->AddChild(MakeLeaf(NodeStatus::Failure));
    sel->AddChild(MakeLeaf(NodeStatus::Failure));
    REQUIRE(sel->Tick(0.f) == NodeStatus::Failure);
}

TEST_CASE("BTSelector returns Running when a child returns Running", "[ai][bt]") {
    auto sel = std::make_shared<BTSelector>();
    sel->AddChild(MakeLeaf(NodeStatus::Failure));
    sel->AddChild(MakeLeaf(NodeStatus::Running));
    REQUIRE(sel->Tick(0.f) == NodeStatus::Running);
}

TEST_CASE("BehaviorTree::Tick delegates to root", "[ai][bt]") {
    BehaviorTree tree;
    tree.SetRoot(MakeLeaf(NodeStatus::Success));
    REQUIRE(tree.Tick(0.016f) == NodeStatus::Success);
}

TEST_CASE("BehaviorTree with no root returns Failure", "[ai][bt]") {
    BehaviorTree tree;
    REQUIRE(tree.Tick(0.f) == NodeStatus::Failure);
}
