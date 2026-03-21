#pragma once
#include <cstdint>
#include <string>
#include <functional>

namespace Core {

/// DeterministicSeed — reproducible seed management for PCG and simulations.
///
/// Provides a central registry of named seeds so that every procedural
/// subsystem (terrain, dungeon, NPC placement, loot tables, etc.) can
/// reproduce identical results given the same session seed.
///
/// Usage:
///   DeterministicSeed::Get().SetMasterSeed(42);
///   uint64_t terrainSeed = DeterministicSeed::Get().Derive("terrain");
class DeterministicSeed {
public:
    static DeterministicSeed& Get();

    // ── master seed ───────────────────────────────────────────
    /// Set the root seed from which all derived seeds are computed.
    void     SetMasterSeed(uint64_t seed);
    /// Generate a new random master seed and return it.
    uint64_t RandomiseMaster();
    uint64_t GetMasterSeed() const;

    // ── derivation ────────────────────────────────────────────
    /// Derive a deterministic child seed for the given named domain.
    /// The result is stable: same master seed + same name → same child seed.
    uint64_t Derive(const std::string& domain) const;
    /// Derive a seed for a specific index within a domain (e.g. chunk coords).
    uint64_t Derive(const std::string& domain, uint64_t index) const;

    // ── serialisation ─────────────────────────────────────────
    /// Export master seed as a short hex string suitable for save-files.
    std::string Serialise() const;
    /// Restore master seed from a hex string returned by Serialise().
    bool        Deserialise(const std::string& hex);

    // ── callbacks ─────────────────────────────────────────────
    /// Fired whenever the master seed changes.
    using SeedChangedCallback = std::function<void(uint64_t newSeed)>;
    void OnSeedChanged(SeedChangedCallback cb);

private:
    DeterministicSeed() = default;

    struct Impl;
    Impl* m_impl{nullptr};
    void  ensureImpl();
};

} // namespace Core
