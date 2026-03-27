#include "Core/Signal/SignalBus/SignalBus.h"
#include <algorithm>
#include <deque>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace Core {

struct Subscription {
    SubscriptionHandle handle;
    std::string        topic;
    SignalHandler      handler;
    int32_t            priority;
    bool               oneShot;
    bool               valid{true};
};

struct SignalBus::Impl {
    std::unordered_map<std::string, std::vector<Subscription>> subs;
    std::deque<std::pair<std::string,Signal>> queue;
    std::unordered_map<std::string, uint64_t> fired;
    std::mutex mtx;
    uint64_t nextHandle{1};
    uint64_t seqCounter{0};
    bool batchMode{false};

    void Dispatch(const std::string& topic, Signal sig){
        fired[topic]++;
        auto it=subs.find(topic);
        if(it!=subs.end()){
            // Sort by priority (ascending) if needed
            auto& list=it->second;
            std::sort(list.begin(),list.end(),[](auto& a,auto& b){return a.priority<b.priority;});
            for(auto& s:list){
                if(s.valid) s.handler(sig);
            }
            // Remove one-shot
            list.erase(std::remove_if(list.begin(),list.end(),
                [](auto& s){return s.oneShot&&s.valid;}),list.end());
        }
        // Wildcard: topic prefix "x.*"
        for(auto& [key,list]:subs){
            if(key==topic) continue;
            if(!key.empty()&&key.back()=='*'){
                std::string prefix=key.substr(0,key.size()-1);
                if(topic.substr(0,prefix.size())==prefix){
                    for(auto& s:list) if(s.valid) s.handler(sig);
                }
            }
        }
    }
};

SignalBus::SignalBus()  : m_impl(new Impl){}
SignalBus::~SignalBus() { Shutdown(); delete m_impl; }
void SignalBus::Init()     {}
void SignalBus::Shutdown() { std::lock_guard<std::mutex> lk(m_impl->mtx); m_impl->subs.clear(); m_impl->queue.clear(); }

SubscriptionHandle SignalBus::Subscribe(const std::string& topic, SignalHandler handler,
                                          int32_t priority, bool oneShot)
{
    std::lock_guard<std::mutex> lk(m_impl->mtx);
    SubscriptionHandle h=m_impl->nextHandle++;
    m_impl->subs[topic].push_back({h,topic,handler,priority,oneShot,true});
    return h;
}

void SignalBus::Unsubscribe(SubscriptionHandle handle){
    std::lock_guard<std::mutex> lk(m_impl->mtx);
    for(auto& [topic,list]:m_impl->subs)
        for(auto& s:list) if(s.handle==handle) s.valid=false;
}
bool SignalBus::IsValid(SubscriptionHandle handle) const {
    for(auto& [topic,list]:m_impl->subs)
        for(auto& s:list) if(s.handle==handle) return s.valid;
    return false;
}

void SignalBus::Fire(const std::string& topic, Signal sig){
    sig.topic=topic; sig.timestamp=++m_impl->seqCounter;
    if(m_impl->batchMode) { m_impl->queue.push_back({topic,sig}); return; }
    std::lock_guard<std::mutex> lk(m_impl->mtx);
    m_impl->Dispatch(topic,sig);
}

void SignalBus::Enqueue(const std::string& topic, Signal sig){
    sig.topic=topic; sig.timestamp=++m_impl->seqCounter;
    std::lock_guard<std::mutex> lk(m_impl->mtx);
    m_impl->queue.push_back({topic,sig});
}

void SignalBus::DispatchAll(){
    std::lock_guard<std::mutex> lk(m_impl->mtx);
    while(!m_impl->queue.empty()){
        auto [topic,sig]=m_impl->queue.front(); m_impl->queue.pop_front();
        m_impl->Dispatch(topic,sig);
    }
}

void     SignalBus::SetBatchMode(bool e){ m_impl->batchMode=e; }
bool     SignalBus::IsBatchMode()  const{ return m_impl->batchMode; }
uint32_t SignalBus::PendingCount() const{ return (uint32_t)m_impl->queue.size(); }

uint64_t SignalBus::FiredCount(const std::string& topic) const {
    auto it=m_impl->fired.find(topic); return it!=m_impl->fired.end()?it->second:0;
}
void SignalBus::ResetStats(){ m_impl->fired.clear(); }

void SignalBus::UnsubscribeAll(const std::string& topic){
    std::lock_guard<std::mutex> lk(m_impl->mtx);
    m_impl->subs.erase(topic);
}
void SignalBus::Clear(){
    std::lock_guard<std::mutex> lk(m_impl->mtx);
    m_impl->subs.clear(); m_impl->queue.clear();
}

} // namespace Core
