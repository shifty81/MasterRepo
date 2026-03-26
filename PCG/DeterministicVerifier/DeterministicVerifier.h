#pragma once
/**
 * @file DeterministicVerifier.h
 * @brief Verifies that PCG outputs are fully reproducible from a given seed.
 *
 * Runs a PCG pipeline twice with the same seed and compares the outputs byte-
 * for-byte (or structurally).  Flags any non-determinism introduced by race
 * conditions, hash-map ordering, or floating-point mode differences.
 *
 * Typical usage:
 * @code
 *   DeterministicVerifier verifier;
 *   verifier.Init();
 *   verifier.RegisterGenerator("terrain", []{ return GenerateTerrain(seed); });
 *   auto result = verifier.Verify("terrain", 12345ULL);
 *   if (!result.passed) Log("Non-determinism: " + result.details);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace PCG {

// ── Result of a determinism verification run ──────────────────────────────────

struct VerifyResult {
    bool        passed{true};
    std::string generatorName;
    uint64_t    seed{0};
    std::string details;          ///< diff description if failed
    uint64_t    runDurationMs{0};
};

// ── DeterministicVerifier ─────────────────────────────────────────────────────

class DeterministicVerifier {
public:
    DeterministicVerifier();
    ~DeterministicVerifier();

    void Init();
    void Shutdown();

    // ── Generator registry ────────────────────────────────────────────────────

    /// Register a named PCG generator that returns a serialisable output string.
    void RegisterGenerator(const std::string& name,
                           std::function<std::string(uint64_t seed)> generator);
    void UnregisterGenerator(const std::string& name);

    // ── Verification ─────────────────────────────────────────────────────────

    /// Run the named generator twice with 'seed' and compare outputs.
    VerifyResult Verify(const std::string& name, uint64_t seed);

    /// Verify all registered generators with a set of seeds.
    std::vector<VerifyResult> VerifyAll(const std::vector<uint64_t>& seeds);

    // ── Seed management ───────────────────────────────────────────────────────

    /// Return a deterministic seed for the given session string.
    static uint64_t MakeSeed(const std::string& sessionKey);

    // ── Callbacks ─────────────────────────────────────────────────────────────

    void OnVerifyFailed(std::function<void(const VerifyResult&)> cb);
    void OnVerifyPassed(std::function<void(const VerifyResult&)> cb);

    // ── History ───────────────────────────────────────────────────────────────

    std::vector<VerifyResult> History()    const;
    void                      ClearHistory();

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace PCG
