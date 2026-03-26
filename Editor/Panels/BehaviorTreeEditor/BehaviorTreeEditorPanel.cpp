#include "Editor/Panels/BehaviorTreeEditor/BehaviorTreeEditorPanel.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <vector>

namespace Editor {

struct BehaviorTreeEditorPanel::Impl {
    std::vector<BTNode>      nodes;
    uint32_t                 nextId{1};
    BTRunState               runState;
    uint32_t                 selectedNode{0};
    float                    scrollX{0.f}, scrollY{0.f};
    float                    zoom{1.f};

    std::function<void(uint32_t)> onSelected;
    std::function<void()>         onChanged;

    BTNode* Find(uint32_t id) {
        for(auto& n:nodes) if(n.id==id) return &n;
        return nullptr;
    }
};

BehaviorTreeEditorPanel::BehaviorTreeEditorPanel() : m_impl(new Impl()) {}
BehaviorTreeEditorPanel::~BehaviorTreeEditorPanel() { delete m_impl; }

void BehaviorTreeEditorPanel::Init()     {}
void BehaviorTreeEditorPanel::Shutdown() { Clear(); }

uint32_t BehaviorTreeEditorPanel::AddNode(BTNodeKind kind, uint32_t parentId,
                                           const std::string& label) {
    BTNode n;
    n.id       = m_impl->nextId++;
    n.kind     = kind;
    n.label    = label.empty() ? "Node" : label;
    n.parentId = parentId;
    n.posX     = 100.f * (float)n.id;
    n.posY     = parentId == 0 ? 50.f : 150.f;

    if (parentId != 0) {
        BTNode* parent = m_impl->Find(parentId);
        if (parent) parent->childIds.push_back(n.id);
    }
    m_impl->nodes.push_back(n);
    if (m_impl->onChanged) m_impl->onChanged();
    return n.id;
}

void BehaviorTreeEditorPanel::RemoveNode(uint32_t id) {
    // Remove from parent's child list
    BTNode* n = m_impl->Find(id);
    if (n && n->parentId) {
        BTNode* parent = m_impl->Find(n->parentId);
        if (parent) {
            auto& c = parent->childIds;
            c.erase(std::remove(c.begin(),c.end(),id),c.end());
        }
    }
    auto& v = m_impl->nodes;
    v.erase(std::remove_if(v.begin(),v.end(),[id](const BTNode& n){ return n.id==id; }),v.end());
    if (m_impl->onChanged) m_impl->onChanged();
}

void BehaviorTreeEditorPanel::MoveNode(uint32_t id, float x, float y) {
    BTNode* n = m_impl->Find(id); if(n){ n->posX=x; n->posY=y; }
}

void BehaviorTreeEditorPanel::SetNodeAction(uint32_t id, const std::string& action) {
    BTNode* n = m_impl->Find(id); if(n) n->actionOrCondition=action;
}

void BehaviorTreeEditorPanel::ReparentNode(uint32_t id, uint32_t newParent) {
    BTNode* n = m_impl->Find(id);
    if (!n) return;
    if (n->parentId) {
        BTNode* old = m_impl->Find(n->parentId);
        if(old){ auto& c=old->childIds; c.erase(std::remove(c.begin(),c.end(),id),c.end()); }
    }
    n->parentId = newParent;
    if (newParent) {
        BTNode* np = m_impl->Find(newParent);
        if(np) np->childIds.push_back(id);
    }
    if (m_impl->onChanged) m_impl->onChanged();
}

BTNode BehaviorTreeEditorPanel::GetNode(uint32_t id) const {
    BTNode* n = m_impl->Find(id); return n ? *n : BTNode{};
}

std::vector<BTNode> BehaviorTreeEditorPanel::AllNodes() const {
    return m_impl->nodes;
}

bool BehaviorTreeEditorPanel::SaveToFile(const std::string& path) const {
    std::ofstream f(path); if(!f.is_open()) return false;
    f<<"{\"nodeCount\":"<<m_impl->nodes.size()<<"}\n";
    return true;
}
bool BehaviorTreeEditorPanel::LoadFromFile(const std::string& path) {
    std::ifstream f(path); return f.is_open();
}
void BehaviorTreeEditorPanel::Clear() { m_impl->nodes.clear(); m_impl->nextId=1; }

void BehaviorTreeEditorPanel::SetRunState(const BTRunState& state) {
    m_impl->runState = state;
    BTNode* n = m_impl->Find(state.activeNodeId);
    if(n) n->isRunning = state.running;
}
void BehaviorTreeEditorPanel::ClearRunState() {
    for(auto& n:m_impl->nodes) n.isRunning=false;
    m_impl->runState = {};
}

void BehaviorTreeEditorPanel::Draw(int,int,int,int) {}
void BehaviorTreeEditorPanel::Tick(float) {}
void BehaviorTreeEditorPanel::OnMouseMove(int,int){}
void BehaviorTreeEditorPanel::OnMouseButton(int,bool,int,int){}
void BehaviorTreeEditorPanel::OnMouseScroll(float,float dy){
    m_impl->zoom = std::max(0.2f, std::min(4.f, m_impl->zoom + dy*0.1f));
}
void BehaviorTreeEditorPanel::OnKey(int,bool){}

void BehaviorTreeEditorPanel::OnNodeSelected(std::function<void(uint32_t)> cb){
    m_impl->onSelected=std::move(cb);
}
void BehaviorTreeEditorPanel::OnTreeChanged(std::function<void()> cb){
    m_impl->onChanged=std::move(cb);
}

} // namespace Editor
