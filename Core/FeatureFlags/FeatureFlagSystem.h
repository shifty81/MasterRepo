#pragma once
/**
 * @file FeatureFlagSystem.h
 * @brief Feature flags and A/B testing for controlled rollout of new features.
 *
 * Flags are loaded from a JSON config file and can be overridden at runtime.
 * Supports:
 *   - Boolean on/off flags
 *   - Percentage rollout (e.g. 10% of players get a new feature)
 *   - String variant flags for A/B experiments
 *   - Remote override (polls a URL if configured; offline-first by default)
 *
 * Typical usage:
 * @code
 *   FeatureFlagSystem flags;
 *   flags.Init("Config/feature_flags.json");
 *   flags.SetUserId("player_42");
 *   if (flags.IsEnabled("new_combat_system")) { ... }
 *   std::string variant = flags.GetVariant("ui_theme", "dark");
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Core {

// ── Flag types ────────────────────────────────────────────────────────────────

enum class FlagType : uint8_t { Boolean = 0, Percentage = 1, Variant = 2 };

// ── A single flag definition ──────────────────────────────────────────────────

struct FeatureFlag {
    std::string id;
    FlagType    type{FlagType::Boolean};
    bool        defaultBool{false};
    float       rolloutPct{0.f};   ///< 0–100 for Percentage type
    std::string defaultVariant;    ///< for Variant type
    std::vector<std::string> variants; ///< candidate variants
    std::string description;
    bool        overridden{false}; ///< set by SetOverride()
};

// ── FeatureFlagSystem ─────────────────────────────────────────────────────────

class FeatureFlagSystem {
public:
    FeatureFlagSystem();
    ~FeatureFlagSystem();

    void Init(const std::string& configPath = "Config/feature_flags.json");
    void Shutdown();

    // ── User context ──────────────────────────────────────────────────────────

    void SetUserId(const std::string& userId);
    std::string GetUserId() const;

    // ── Flag queries ──────────────────────────────────────────────────────────

    bool        IsEnabled(const std::string& flagId) const;
    std::string GetVariant(const std::string& flagId,
                           const std::string& defaultValue = "") const;
    float       GetRolloutPct(const std::string& flagId) const;

    // ── Flag registration ─────────────────────────────────────────────────────

    void DefineFlag(const FeatureFlag& flag);
    void DefineBoolFlag(const std::string& id, bool defaultValue,
                        const std::string& description = "");
    void DefinePercentFlag(const std::string& id, float rolloutPct,
                           const std::string& description = "");
    void DefineVariantFlag(const std::string& id,
                           const std::vector<std::string>& variants,
                           const std::string& defaultVariant,
                           const std::string& description = "");

    bool HasFlag(const std::string& id) const;
    FeatureFlag GetFlag(const std::string& id) const;
    std::vector<FeatureFlag> AllFlags() const;

    // ── Runtime overrides (developer / QA use) ────────────────────────────────

    void SetOverrideBool(const std::string& id, bool value);
    void SetOverrideVariant(const std::string& id, const std::string& variant);
    void ClearOverride(const std::string& id);
    void ClearAllOverrides();

    // ── Persistence ───────────────────────────────────────────────────────────

    void Save() const;
    void Load();

    // ── Callbacks ─────────────────────────────────────────────────────────────

    void OnFlagChanged(std::function<void(const std::string& id,
                                          bool newValue)> cb);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Core
