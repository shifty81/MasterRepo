#include "Runtime/Gameplay/Loot/LootTable/LootTable.h"
#include <algorithm>
#include <cstdlib>
#include <unordered_map>

namespace Runtime {

struct PoolEntry { LootPool pool; std::vector<LootEntry> overrides; };

struct LootTable::Impl {
    std::unordered_map<std::string,PoolEntry> pools;
    float luckMul{1.f};

    uint32_t lcg(uint32_t& s) const { s=1664525u*s+1013904223u; return s; }

    std::vector<LootResult> DoRoll(const std::string& pid, uint32_t seed, uint32_t n) const {
        auto it=pools.find(pid); if(it==pools.end()) return {};
        const auto& pe=it->second;
        std::vector<LootEntry> entries=pe.pool.entries;
        for(auto& o:pe.overrides) entries.push_back(o);
        // Apply luck to higher tiers
        for(auto& e:entries) if((uint8_t)e.rarity>0) e.weight*=(1.f+luckMul*(uint8_t)e.rarity*0.1f);

        float totalW=0; for(auto& e:entries) totalW+=e.weight;
        std::vector<LootResult> results;
        // Guaranteed
        for(auto& g:pe.pool.guaranteed){
            for(auto& e:entries) if(e.id==g){ results.push_back({e.id,e.minQty,e.rarity}); break; }
        }
        uint32_t s=seed?seed:12345u;
        uint32_t rolls=pe.pool.rollCount*n;
        for(uint32_t r=0;r<rolls;r++){
            float v=(float)(lcg(s)&0xFFFF)/65535.f*totalW;
            float acc=0;
            for(auto& e:entries){
                acc+=e.weight; if(v<acc){
                    uint32_t qty=e.minQty;
                    if(e.maxQty>e.minQty) qty=e.minQty+(lcg(s)%(e.maxQty-e.minQty+1));
                    results.push_back({e.id,qty,e.rarity}); break;
                }
            }
        }
        return results;
    }
};

LootTable::LootTable() : m_impl(new Impl){}
LootTable::~LootTable(){ Shutdown(); delete m_impl; }
void LootTable::Init()     {}
void LootTable::Shutdown() { m_impl->pools.clear(); }

void LootTable::RegisterPool  (const std::string& id, const LootPool& pool){ m_impl->pools[id]={pool,{}}; }
void LootTable::UnregisterPool(const std::string& id){ m_impl->pools.erase(id); }
const LootPool* LootTable::GetPool(const std::string& id) const { auto it=m_impl->pools.find(id); return it!=m_impl->pools.end()?&it->second.pool:nullptr; }

std::vector<LootResult> LootTable::Roll     (const std::string& id, uint32_t seed) const { return m_impl->DoRoll(id,seed,1); }
std::vector<LootResult> LootTable::MultiRoll(const std::string& id, uint32_t n, uint32_t seed) const { return m_impl->DoRoll(id,seed,n); }

void LootTable::SetGlobalLuckMultiplier(float f){ m_impl->luckMul=f; }
void LootTable::AddDropOverride   (const std::string& id, const LootEntry& e){ m_impl->pools[id].overrides.push_back(e); }
void LootTable::ClearDropOverrides(const std::string& id){ auto it=m_impl->pools.find(id); if(it!=m_impl->pools.end()) it->second.overrides.clear(); }

std::vector<std::pair<std::string,float>> LootTable::SimulateDropRates(const std::string& id, uint32_t n) const {
    auto results=m_impl->DoRoll(id,1,n);
    std::unordered_map<std::string,uint32_t> counts;
    for(auto& r:results) counts[r.itemId]++;
    std::vector<std::pair<std::string,float>> rates;
    for(auto& [k,v]:counts) rates.push_back({k,(float)v/n});
    return rates;
}
std::vector<std::string> LootTable::GetAllPoolIds() const { std::vector<std::string> v; for(auto& [k,_]:m_impl->pools) v.push_back(k); return v; }

} // namespace Runtime
