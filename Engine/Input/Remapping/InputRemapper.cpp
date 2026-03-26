#include "Engine/Input/Remapping/InputRemapper.h"
#include <algorithm>
#include <fstream>
#include <unordered_map>
#include <vector>

namespace Engine {

struct InputRemapper::Impl {
    std::unordered_map<std::string, InputAction> actions;

    // Raw state (current frame)
    std::unordered_map<uint32_t, bool>            keysCurr;
    std::unordered_map<uint8_t,  bool>            mouseCurr;
    std::unordered_map<uint8_t,  bool>            gpBtnCurr;
    std::unordered_map<uint8_t,  float>           gpAxes;

    // Previous frame
    std::unordered_map<uint32_t, bool>            keysPrev;
    std::unordered_map<uint8_t,  bool>            mousePrev;
    std::unordered_map<uint8_t,  bool>            gpBtnPrev;

    std::function<void(const std::string&)> onChanged;

    bool IsBindingActive(const InputBinding& b) const {
        switch(b.type){
            case BindingType::Key:
                return keysCurr.count((uint32_t)b.key) && keysCurr.at((uint32_t)b.key);
            case BindingType::MouseBtn:
                return mouseCurr.count((uint8_t)b.mouseBtn) && mouseCurr.at((uint8_t)b.mouseBtn);
            case BindingType::GamepadBtn:
                return gpBtnCurr.count((uint8_t)b.gpBtn) && gpBtnCurr.at((uint8_t)b.gpBtn);
            case BindingType::GamepadAxis: {
                auto it=gpAxes.find((uint8_t)b.gpAxis);
                return it!=gpAxes.end() && std::abs(it->second) > b.axisDeadzone;
            }
        }
        return false;
    }
    bool WasBindingActive(const InputBinding& b) const {
        switch(b.type){
            case BindingType::Key:       return keysPrev.count((uint32_t)b.key)    && keysPrev.at((uint32_t)b.key);
            case BindingType::MouseBtn:  return mousePrev.count((uint8_t)b.mouseBtn) && mousePrev.at((uint8_t)b.mouseBtn);
            case BindingType::GamepadBtn:return gpBtnPrev.count((uint8_t)b.gpBtn)  && gpBtnPrev.at((uint8_t)b.gpBtn);
            default: return false;
        }
    }
    float GetBindingAxis(const InputBinding& b) const {
        if(b.type==BindingType::GamepadAxis){
            auto it=gpAxes.find((uint8_t)b.gpAxis);
            if(it!=gpAxes.end() && std::abs(it->second)>b.axisDeadzone) return it->second*b.axisScale;
        }
        return IsBindingActive(b) ? b.axisScale : 0.f;
    }
};

InputRemapper::InputRemapper() : m_impl(new Impl()) {}
InputRemapper::~InputRemapper() { delete m_impl; }
void InputRemapper::Init()     {}
void InputRemapper::Shutdown() {}

void InputRemapper::RegisterAction(const std::string& name, bool isAxis){
    if(!m_impl->actions.count(name)){ InputAction a; a.name=name; a.isAxis=isAxis; m_impl->actions[name]=a; }
}
void InputRemapper::UnregisterAction(const std::string& name){ m_impl->actions.erase(name); }
bool InputRemapper::HasAction(const std::string& name) const{ return m_impl->actions.count(name)>0; }
std::vector<InputAction> InputRemapper::AllActions() const{
    std::vector<InputAction> out; for(auto&[k,v]:m_impl->actions) out.push_back(v); return out;
}

static void AddOrReplace(InputAction& a, InputBinding b){
    for(auto& existing:a.bindings) if(existing.isPrimary==b.isPrimary&&existing.type==b.type){ existing=b; return; }
    a.bindings.push_back(b);
}
void InputRemapper::BindKey(const std::string& action, KeyCode key, bool primary){
    auto it=m_impl->actions.find(action); if(it==m_impl->actions.end()) return;
    InputBinding b; b.type=BindingType::Key; b.key=key; b.isPrimary=primary;
    AddOrReplace(it->second,b);
    if(m_impl->onChanged) m_impl->onChanged(action);
}
void InputRemapper::BindMouseButton(const std::string& action, MouseButton btn, bool primary){
    auto it=m_impl->actions.find(action); if(it==m_impl->actions.end()) return;
    InputBinding b; b.type=BindingType::MouseBtn; b.mouseBtn=btn; b.isPrimary=primary;
    AddOrReplace(it->second,b);
    if(m_impl->onChanged) m_impl->onChanged(action);
}
void InputRemapper::BindGamepadButton(const std::string& action, GamepadButton btn, bool primary){
    auto it=m_impl->actions.find(action); if(it==m_impl->actions.end()) return;
    InputBinding b; b.type=BindingType::GamepadBtn; b.gpBtn=btn; b.isPrimary=primary;
    AddOrReplace(it->second,b);
    if(m_impl->onChanged) m_impl->onChanged(action);
}
void InputRemapper::BindGamepadAxis(const std::string& action, GamepadAxis axis, float scale, bool primary){
    auto it=m_impl->actions.find(action); if(it==m_impl->actions.end()) return;
    InputBinding b; b.type=BindingType::GamepadAxis; b.gpAxis=axis; b.axisScale=scale; b.isPrimary=primary;
    AddOrReplace(it->second,b);
    if(m_impl->onChanged) m_impl->onChanged(action);
}
void InputRemapper::ClearBindings(const std::string& action){
    auto it=m_impl->actions.find(action); if(it!=m_impl->actions.end()) it->second.bindings.clear();
}

bool InputRemapper::IsPressed(const std::string& action) const{
    auto it=m_impl->actions.find(action); if(it==m_impl->actions.end()) return false;
    for(auto& b:it->second.bindings) if(m_impl->IsBindingActive(b)&&!m_impl->WasBindingActive(b)) return true;
    return false;
}
bool InputRemapper::IsHeld(const std::string& action) const{
    auto it=m_impl->actions.find(action); if(it==m_impl->actions.end()) return false;
    for(auto& b:it->second.bindings) if(m_impl->IsBindingActive(b)) return true;
    return false;
}
bool InputRemapper::IsReleased(const std::string& action) const{
    auto it=m_impl->actions.find(action); if(it==m_impl->actions.end()) return false;
    for(auto& b:it->second.bindings) if(!m_impl->IsBindingActive(b)&&m_impl->WasBindingActive(b)) return true;
    return false;
}
float InputRemapper::GetAxis(const std::string& action) const{
    auto it=m_impl->actions.find(action); if(it==m_impl->actions.end()) return 0.f;
    for(auto& b:it->second.bindings){ float v=m_impl->GetBindingAxis(b); if(v!=0.f) return v; }
    return 0.f;
}

void InputRemapper::SetKeyState(KeyCode key, bool pressed){ m_impl->keysCurr[(uint32_t)key]=pressed; }
void InputRemapper::SetMouseButtonState(MouseButton btn, bool pressed){ m_impl->mouseCurr[(uint8_t)btn]=pressed; }
void InputRemapper::SetGamepadButtonState(uint8_t, GamepadButton btn, bool pressed){ m_impl->gpBtnCurr[(uint8_t)btn]=pressed; }
void InputRemapper::SetGamepadAxisValue(uint8_t, GamepadAxis axis, float v){ m_impl->gpAxes[(uint8_t)axis]=v; }

void InputRemapper::Update(){
    m_impl->keysPrev   = m_impl->keysCurr;
    m_impl->mousePrev  = m_impl->mouseCurr;
    m_impl->gpBtnPrev  = m_impl->gpBtnCurr;
}

std::vector<std::string> InputRemapper::FindConflicts() const{
    std::vector<std::string> out;
    std::unordered_map<std::string,std::string> seen; // binding-key → action
    for(auto&[aname,action]:m_impl->actions)
        for(auto& b:action.bindings){
            std::string key=std::to_string((int)b.type)+":"+std::to_string((int)b.key);
            auto it=seen.find(key);
            if(it!=seen.end()&&it->second!=aname) out.push_back(aname+" conflicts with "+it->second);
            else seen[key]=aname;
        }
    return out;
}

bool InputRemapper::SaveProfile(const std::string& path) const{
    std::ofstream f(path); if(!f.is_open()) return false;
    for(auto&[n,a]:m_impl->actions) f<<n<<":"<<a.bindings.size()<<"\n";
    return true;
}
bool InputRemapper::LoadProfile(const std::string& path){
    std::ifstream f(path); return f.is_open();
}
void InputRemapper::ResetToDefaults(){
    for(auto&[k,v]:m_impl->actions) v.bindings.clear();
}
void InputRemapper::OnBindingChanged(std::function<void(const std::string&)> cb){ m_impl->onChanged=std::move(cb); }

} // namespace Engine
