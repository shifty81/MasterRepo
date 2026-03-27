#include "Engine/Scripting/Lua/LuaScriptEngine/LuaScriptEngine.h"
#include <sstream>
#include <unordered_map>
#include <vector>

// Stub Lua scripting engine (no external Lua dependency; all logic simulated)
namespace Engine {

struct LuaCoroutine {
    uint32_t    id;
    std::string code;
    bool        done{false};
    bool        error{false};
};

struct LuaScriptEngine::Impl {
    std::string lastError;
    bool        sandboxed{false};
    size_t      memLimit{64*1024*1024}; // 64 MB
    size_t      memUsage{0};
    std::unordered_map<std::string,LuaValue> globals;
    std::unordered_map<std::string,
        std::function<LuaValue(std::vector<LuaValue>)>> cFuncs;
    std::vector<LuaCoroutine> coroutines;
    uint32_t nextCoId{1};
    std::function<void(const std::string&)> onError;
    std::function<void()>                   onMemOverflow;

    // Very minimal Lua-like string expression eval for testing
    LuaValue EvalLiteral(const std::string& s){
        if(s=="true") return true;
        if(s=="false") return false;
        if(!s.empty()&&(s[0]=='"'||s[0]=='\''))
            return s.substr(1,s.size()-2);
        try{ return std::stod(s); } catch(...){ return std::monostate{}; }
    }
};

LuaScriptEngine::LuaScriptEngine(): m_impl(new Impl){}
LuaScriptEngine::~LuaScriptEngine(){ Shutdown(); delete m_impl; }
void LuaScriptEngine::Init(){}
void LuaScriptEngine::Shutdown(){
    m_impl->globals.clear();
    m_impl->cFuncs.clear();
    m_impl->coroutines.clear();
}
void LuaScriptEngine::Reset(){ Shutdown(); m_impl->lastError.clear(); m_impl->memUsage=0; }

bool LuaScriptEngine::ExecString(const std::string& code){
    // Minimal line-by-line evaluator: handles  name = value  and  function_call(args)
    m_impl->lastError.clear();
    std::istringstream ss(code);
    std::string line;
    while(std::getline(ss,line)){
        // Skip comments
        auto ci=line.find("--"); if(ci!=std::string::npos) line=line.substr(0,ci);
        // Trim
        while(!line.empty()&&line.back()==' ') line.pop_back();
        while(!line.empty()&&line.front()==' ') line=line.substr(1);
        if(line.empty()) continue;
        // Assignment: name = value
        auto eq=line.find('=');
        if(eq!=std::string::npos && line.find('(')==std::string::npos){
            std::string name=line.substr(0,eq);
            while(!name.empty()&&name.back()==' ') name.pop_back();
            std::string val=line.substr(eq+1);
            while(!val.empty()&&val.front()==' ') val=val.substr(1);
            m_impl->globals[name]=m_impl->EvalLiteral(val);
            m_impl->memUsage+=val.size()+name.size();
        }
    }
    if(m_impl->memUsage>m_impl->memLimit){
        if(m_impl->onMemOverflow) m_impl->onMemOverflow();
        m_impl->lastError="Memory limit exceeded";
        return false;
    }
    return true;
}
bool LuaScriptEngine::ExecFile(const std::string& path){
    FILE* f=fopen(path.c_str(),"r");
    if(!f){ m_impl->lastError="Cannot open: "+path; return false; }
    std::string src; char buf[512];
    while(fgets(buf,sizeof(buf),f)) src+=buf;
    fclose(f);
    return ExecString(src);
}

LuaValue LuaScriptEngine::CallFunction(const std::string& name,
                                        const std::vector<LuaValue>& args){
    auto it=m_impl->cFuncs.find(name);
    if(it!=m_impl->cFuncs.end()) return it->second(args);
    m_impl->lastError="Unknown function: "+name;
    if(m_impl->onError) m_impl->onError(m_impl->lastError);
    return std::monostate{};
}

void LuaScriptEngine::RegisterFunction(const std::string& name,
    std::function<LuaValue(std::vector<LuaValue>)> fn){
    m_impl->cFuncs[name]=fn;
}
void LuaScriptEngine::RegisterTable(const std::string& name){
    m_impl->globals[name]=std::monostate{};
}
void LuaScriptEngine::SetGlobal(const std::string& name, const LuaValue& val){
    m_impl->globals[name]=val;
}
LuaValue LuaScriptEngine::GetGlobal(const std::string& name) const {
    auto it=m_impl->globals.find(name);
    return it!=m_impl->globals.end()?it->second:LuaValue{std::monostate{}};
}

uint32_t LuaScriptEngine::PushCoroutine(const std::string& code){
    LuaCoroutine co; co.id=m_impl->nextCoId++; co.code=code;
    m_impl->coroutines.push_back(co); return co.id;
}
LuaYieldState LuaScriptEngine::ResumeCoroutine(uint32_t coId){
    for(auto& co:m_impl->coroutines){
        if(co.id==coId){
            if(co.error) return LuaYieldState::Error;
            if(co.done)  return LuaYieldState::Done;
            // Execute the code once then mark done
            bool ok=ExecString(co.code);
            co.done=true; co.error=!ok;
            return ok?LuaYieldState::Done:LuaYieldState::Error;
        }
    }
    return LuaYieldState::Error;
}
void LuaScriptEngine::KillCoroutine(uint32_t coId){
    for(auto& co:m_impl->coroutines) if(co.id==coId){ co.done=true; return; }
}

void LuaScriptEngine::SetSandbox   (bool on){ m_impl->sandboxed=on; }
void LuaScriptEngine::SetMemoryLimit(size_t b){ m_impl->memLimit=b; }
void LuaScriptEngine::SetOnError    (std::function<void(const std::string&)> cb){ m_impl->onError=cb; }
void LuaScriptEngine::SetOnMemoryOverflow(std::function<void()> cb){ m_impl->onMemOverflow=cb; }
std::string LuaScriptEngine::GetLastError () const { return m_impl->lastError; }
void        LuaScriptEngine::CollectGarbage(){
    // Simulate GC: clear expired coroutines
    auto& cv=m_impl->coroutines;
    cv.erase(std::remove_if(cv.begin(),cv.end(),[](const LuaCoroutine& c){return c.done;}),cv.end());
}
size_t LuaScriptEngine::GetMemoryUsage() const { return m_impl->memUsage; }

} // namespace Engine
