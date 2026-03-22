#pragma once
/**
 * @file AIWalletSystem.h
 * @brief NPC credits economy — wallet management with deposit, withdraw, transfer.
 *
 * Adapted from Nova-Forge-Expeditions (atlas::sim → Runtime namespace).
 *
 * AIWalletSystem manages per-entity credit wallets for the in-game economy:
 *   - CreateWallet(entityId, initialBalance) / RemoveWallet(entityId)
 *   - Deposit(entityId, amount) / Withdraw(entityId, amount)
 *   - Transfer(fromId, toId, amount): atomic, fails if insufficient funds
 *   - GetWallet(entityId) / GetBalance(entityId)
 *   - TotalCirculation(): sum of all balances
 *   - WealthiestEntity(): entityId with the highest balance
 *   - Clear()
 */

#include <cstdint>
#include <vector>

namespace Runtime {

// ── Wallet ────────────────────────────────────────────────────────────────
struct AIWallet {
    uint32_t entityId         = 0;
    float    balance          = 0.0f;
    float    totalIncome      = 0.0f;
    float    totalExpenses    = 0.0f;
    uint32_t transactionCount = 0;
};

// ── System ────────────────────────────────────────────────────────────────
class AIWalletSystem {
public:
    AIWalletSystem() = default;

    bool CreateWallet(uint32_t entityId, float initialBalance = 0.0f);
    bool RemoveWallet(uint32_t entityId);

    bool Deposit(uint32_t entityId, float amount);
    bool Withdraw(uint32_t entityId, float amount);
    bool Transfer(uint32_t fromId, uint32_t toId, float amount);

    const AIWallet* GetWallet(uint32_t entityId) const;
    float           GetBalance(uint32_t entityId) const;

    size_t   WalletCount()      const { return m_wallets.size(); }
    float    TotalCirculation() const;
    uint32_t WealthiestEntity() const;
    void     Clear();

private:
    AIWallet* findWallet(uint32_t entityId);

    std::vector<AIWallet> m_wallets;
};

} // namespace Runtime
