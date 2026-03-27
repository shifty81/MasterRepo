#include "Runtime/Economy/EconomySystem/EconomySystem.h"
#include <algorithm>
#include <unordered_map>
#include <vector>

namespace Runtime {

struct EconomySystem::Impl {
    std::unordered_map<std::string,ItemDef>   items;
    std::unordered_map<uint32_t,float>        balances;
    // sellerId → (itemId → qty)
    std::unordered_map<uint32_t,std::unordered_map<std::string,uint32_t>> stock;
    // item price multipliers
    std::unordered_map<std::string,float> itemMul;
    std::unordered_map<uint32_t,float>    discounts;
    float globalMul{1.f};
    float taxRate  {0.f};
    float time     {0.f};
    std::vector<Transaction> log;
    uint32_t logSize{100};
    std::function<void(const Transaction&)> onTx;
};

EconomySystem::EconomySystem()  : m_impl(new Impl){}
EconomySystem::~EconomySystem() { Shutdown(); delete m_impl; }
void EconomySystem::Init()     {}
void EconomySystem::Shutdown() { m_impl->items.clear(); m_impl->stock.clear(); m_impl->balances.clear(); }

void EconomySystem::RegisterItem  (const ItemDef& d){ m_impl->items[d.id]=d; }
void EconomySystem::UnregisterItem(const std::string& id){ m_impl->items.erase(id); }
const ItemDef* EconomySystem::GetItem(const std::string& id) const {
    auto it=m_impl->items.find(id); return it!=m_impl->items.end()?&it->second:nullptr;
}

void     EconomySystem::SetStockLevel(uint32_t s, const std::string& id, uint32_t q){ m_impl->stock[s][id]=q; }
uint32_t EconomySystem::GetStockLevel(uint32_t s, const std::string& id) const {
    auto it=m_impl->stock.find(s); if(it==m_impl->stock.end()) return 0;
    auto it2=it->second.find(id); return it2!=it->second.end()?it2->second:0;
}

void  EconomySystem::AddCurrency   (uint32_t id, float a){ m_impl->balances[id]+=a; }
bool  EconomySystem::RemoveCurrency(uint32_t id, float a){
    if(m_impl->balances[id]<a) return false; m_impl->balances[id]-=a; return true;
}
float EconomySystem::GetBalance(uint32_t id) const {
    auto it=m_impl->balances.find(id); return it!=m_impl->balances.end()?it->second:0.f;
}

float EconomySystem::GetPrice(uint32_t /*seller*/, const std::string& itemId) const {
    auto* def=GetItem(itemId); if(!def) return 0.f;
    float p=def->basePrice*m_impl->globalMul;
    auto it=m_impl->itemMul.find(itemId); if(it!=m_impl->itemMul.end()) p*=it->second;
    return p;
}

bool EconomySystem::Buy(uint32_t buyer, uint32_t seller, const std::string& itemId, uint32_t qty){
    float price=GetPrice(seller,itemId)*qty;
    float disc=0; auto dit=m_impl->discounts.find(buyer); if(dit!=m_impl->discounts.end()) disc=dit->second;
    price*=(1.f-disc);
    if(!RemoveCurrency(buyer,price)) return false;
    if(GetStockLevel(seller,itemId)<qty) return false;
    m_impl->stock[seller][itemId]-=qty;
    float tax=price*m_impl->taxRate;
    AddCurrency(seller, price-tax);
    Transaction tx; tx.sellerId=seller; tx.buyerId=buyer; tx.itemId=itemId;
    tx.quantity=qty; tx.totalPrice=price; tx.timestamp=m_impl->time;
    m_impl->log.push_back(tx);
    if((uint32_t)m_impl->log.size()>m_impl->logSize) m_impl->log.erase(m_impl->log.begin());
    if(m_impl->onTx) m_impl->onTx(tx);
    return true;
}
bool EconomySystem::Sell(uint32_t seller, uint32_t buyer, const std::string& itemId, uint32_t qty){
    return Buy(buyer, seller, itemId, qty);
}

void EconomySystem::SetGlobalPriceMultiplier(float m){ m_impl->globalMul=m; }
void EconomySystem::SetItemPriceMultiplier(const std::string& id, float m){ m_impl->itemMul[id]=m; }
void EconomySystem::SetEntityDiscount(uint32_t id, float d){ m_impl->discounts[id]=d; }
void EconomySystem::SetTaxRate(float r){ m_impl->taxRate=r; }

const std::vector<Transaction>& EconomySystem::GetTransactionLog() const { return m_impl->log; }
void EconomySystem::SetTransactionLogSize(uint32_t n){ m_impl->logSize=n; }
void EconomySystem::SetOnTransaction(std::function<void(const Transaction&)> cb){ m_impl->onTx=cb; }

} // namespace Runtime
