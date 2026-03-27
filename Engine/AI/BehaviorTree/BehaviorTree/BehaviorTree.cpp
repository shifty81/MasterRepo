#include "Engine/AI/BehaviorTree/BehaviorTree/BehaviorTree.h"
#include <fstream>

namespace Engine {

// ── BehaviorTree ─────────────────────────────────────────────────────────────

uint32_t BehaviorTree::CreateInstance()
{
    Instance inst;
    inst.id = m_nextId++;
    m_instances.push_back(std::move(inst));
    return m_instances.back().id;
}

void BehaviorTree::DestroyInstance(uint32_t id)
{
    auto& v = m_instances;
    v.erase(std::remove_if(v.begin(),v.end(),[&](auto& i){ return i.id==id; }), v.end());
}

BTStatus BehaviorTree::Tick(uint32_t instanceId, float dt)
{
    if (!m_root) return BTStatus::Failure;
    for (auto& inst : m_instances) {
        if (inst.id != instanceId) continue;
        auto status = m_root->Tick(inst.bb, dt);
        if (m_onTick) m_onTick(m_root->Name(), status);
        return status;
    }
    return BTStatus::Failure;
}

void BehaviorTree::Reset(uint32_t instanceId)
{
    for (auto& inst : m_instances) {
        if (inst.id==instanceId && m_root) { m_root->Reset(inst.bb); return; }
    }
}

Blackboard& BehaviorTree::GetBlackboard(uint32_t instanceId)
{
    for (auto& inst : m_instances) if (inst.id==instanceId) return inst.bb;
    static Blackboard dummy;
    return dummy;
}

bool BehaviorTree::LoadFromJSON(const std::string& path)
{
    std::ifstream f(path); return f.good();
}

// ── BehaviorTreeBuilder ───────────────────────────────────────────────────────

void BehaviorTreeBuilder::Push(std::shared_ptr<BTNode> node)
{
    if (!m_stack.empty()) m_stack.back()->children.push_back(node);
    else                   m_root = node;
    m_stack.push_back(node);
}

BehaviorTreeBuilder& BehaviorTreeBuilder::Sequence(const std::string& name)
{ Push(std::make_shared<BTSequence>(name)); return *this; }

BehaviorTreeBuilder& BehaviorTreeBuilder::Selector(const std::string& name)
{ Push(std::make_shared<BTSelector>(name)); return *this; }

BehaviorTreeBuilder& BehaviorTreeBuilder::Parallel(const std::string& name, uint32_t thresh)
{ Push(std::make_shared<BTParallel>(name, thresh)); return *this; }

BehaviorTreeBuilder& BehaviorTreeBuilder::Inverter(const std::string& name)
{ Push(std::make_shared<BTInverter>(name)); return *this; }

BehaviorTreeBuilder& BehaviorTreeBuilder::Repeater(const std::string& name, uint32_t times)
{ Push(std::make_shared<BTRepeater>(name, times)); return *this; }

BehaviorTreeBuilder& BehaviorTreeBuilder::Action(const std::string& name, BTAction::ActionFn fn)
{
    auto node = std::make_shared<BTAction>(name, fn);
    if (!m_stack.empty()) m_stack.back()->children.push_back(node);
    else                   m_root = node;
    return *this;
}

BehaviorTreeBuilder& BehaviorTreeBuilder::Condition(const std::string& name, BTCondition::CondFn fn)
{
    auto node = std::make_shared<BTCondition>(name, fn);
    if (!m_stack.empty()) m_stack.back()->children.push_back(node);
    else                   m_root = node;
    return *this;
}

BehaviorTreeBuilder& BehaviorTreeBuilder::Wait(const std::string& name, float duration)
{
    auto node = std::make_shared<BTWait>(name, duration);
    if (!m_stack.empty()) m_stack.back()->children.push_back(node);
    else                   m_root = node;
    return *this;
}

BehaviorTreeBuilder& BehaviorTreeBuilder::End()
{
    if (!m_stack.empty()) m_stack.pop_back();
    return *this;
}

std::shared_ptr<BehaviorTree> BehaviorTreeBuilder::Build()
{
    auto tree = std::make_shared<BehaviorTree>();
    tree->SetRoot(m_root);
    return tree;
}

} // namespace Engine
