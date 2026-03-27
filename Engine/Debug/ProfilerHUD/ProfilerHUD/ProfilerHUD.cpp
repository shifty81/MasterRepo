#include "Engine/Debug/ProfilerHUD/ProfilerHUD/ProfilerHUD.h"
#include <algorithm>
#include <chrono>
#include <deque>
#include <sstream>
#include <unordered_map>

namespace Engine {

using Clock = std::chrono::high_resolution_clock;
using TP    = std::chrono::time_point<Clock>;

struct ScopeData {
    std::string name;
    float       lastMs{0};
    float       avgMs {0};
    float       maxMs {0};
    float       budgetMs{0};
    TP          startTime{};
    bool        open{false};
    std::deque<float> history;
};

struct ProfilerHUD::Impl {
    float   frameBudgetMs{16.67f};
    uint32_t historyLen{120};
    float   frameDt{0};
    float   fps{0};
    TP      frameStart{};
    std::unordered_map<std::string,ScopeData> scopes;
};

ProfilerHUD::ProfilerHUD(): m_impl(new Impl){}
ProfilerHUD::~ProfilerHUD(){ Shutdown(); delete m_impl; }

void ProfilerHUD::Init(float budgetMs){ m_impl->frameBudgetMs=budgetMs; m_impl->frameStart=Clock::now(); }
void ProfilerHUD::Shutdown(){ m_impl->scopes.clear(); }
void ProfilerHUD::Reset(){ m_impl->scopes.clear(); m_impl->frameDt=0; m_impl->fps=0; }

void ProfilerHUD::NewFrame(){
    auto now=Clock::now();
    float dt=std::chrono::duration<float,std::milli>(now-m_impl->frameStart).count();
    m_impl->frameStart=now;
    m_impl->frameDt=dt;
    m_impl->fps=dt>0?1000.f/dt:0;
    // Rotate history for all scopes
    for(auto& [n,s]:m_impl->scopes){
        s.history.push_back(s.lastMs);
        while((uint32_t)s.history.size()>m_impl->historyLen) s.history.pop_front();
        // Update avg and max
        float sum=0,mx=0;
        for(float v:s.history){ sum+=v; if(v>mx) mx=v; }
        s.avgMs=sum/(float)s.history.size();
        s.maxMs=mx;
    }
}

void ProfilerHUD::BeginScope(const std::string& name){
    auto& s=m_impl->scopes[name];
    s.name=name;
    s.startTime=Clock::now();
    s.open=true;
}
void ProfilerHUD::EndScope(const std::string& name){
    auto it=m_impl->scopes.find(name);
    if(it==m_impl->scopes.end()||!it->second.open) return;
    auto now=Clock::now();
    it->second.lastMs=std::chrono::duration<float,std::milli>(now-it->second.startTime).count();
    it->second.open=false;
}

float ProfilerHUD::GetScopeMs(const std::string& name) const {
    auto it=m_impl->scopes.find(name);
    return it!=m_impl->scopes.end()?it->second.lastMs:0.f;
}

void ProfilerHUD::GetScopeHistory(const std::string& name, float* out, uint32_t count) const {
    auto it=m_impl->scopes.find(name);
    if(it==m_impl->scopes.end()){ for(uint32_t i=0;i<count;i++) out[i]=0; return; }
    auto& h=it->second.history;
    uint32_t n=(uint32_t)h.size();
    for(uint32_t i=0;i<count;i++){
        // Fill oldest-first; zero-pad if history is shorter than requested count
        uint32_t histIdx=(count>=n)?(i<(count-n)?UINT32_MAX:i-(count-n)):(n-count+i);
        out[i]=(histIdx==UINT32_MAX||histIdx>=n)?0.f:h[histIdx];
    }
}

std::vector<ScopeInfo> ProfilerHUD::GetAllScopes() const {
    std::vector<ScopeInfo> out;
    for(auto& [n,s]:m_impl->scopes){
        ScopeInfo si; si.name=s.name; si.lastMs=s.lastMs;
        si.avgMs=s.avgMs; si.maxMs=s.maxMs; si.budgetMs=s.budgetMs;
        out.push_back(si);
    }
    return out;
}

float ProfilerHUD::GetFrameMs() const { return m_impl->frameDt; }
float ProfilerHUD::GetFPS    () const { return m_impl->fps; }

void ProfilerHUD::SetBudgetMs(const std::string& name, float ms){
    m_impl->scopes[name].budgetMs=ms;
}

std::vector<ProfilerBar> ProfilerHUD::RenderBars() const {
    std::vector<ProfilerBar> bars;
    float budget=m_impl->frameBudgetMs; if(budget<=0) return bars;
    float accum=0;
    for(auto& [n,s]:m_impl->scopes){
        ProfilerBar b;
        b.name=s.name;
        b.startFrac=accum/budget;
        b.widthFrac=s.lastMs/budget;
        b.overBudget=(s.budgetMs>0&&s.lastMs>s.budgetMs);
        bars.push_back(b);
        accum+=s.lastMs;
    }
    return bars;
}

void ProfilerHUD::SetHistoryLength(uint32_t n){ m_impl->historyLen=n; }
void ProfilerHUD::SetFrameBudgetMs(float ms)  { m_impl->frameBudgetMs=ms; }

std::string ProfilerHUD::ExportJSON() const {
    std::ostringstream os; os<<"{\"fps\":"<<m_impl->fps<<",\"frameMs\":"<<m_impl->frameDt<<",\"scopes\":[";
    bool first=true;
    for(auto& [n,s]:m_impl->scopes){
        if(!first) os<<",";
        first=false;
        os<<"{\"name\":\""<<s.name<<"\",\"lastMs\":"<<s.lastMs
          <<",\"avgMs\":"<<s.avgMs<<",\"maxMs\":"<<s.maxMs<<"}";
    }
    os<<"]}";
    return os.str();
}

} // namespace Engine
