#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <functional>

namespace PCG {

// ──────────────────────────────────────────────────────────────
// Validation result
// ──────────────────────────────────────────────────────────────

enum class ValidationSeverity { Info, Warning, Error };

struct ValidationIssue {
    ValidationSeverity  severity = ValidationSeverity::Info;
    std::string         category; // e.g. "geometry", "quest", "audio"
    std::string         message;
    std::string         nodeID;   // optional: which node/id triggered this
};

struct ValidationReport {
    bool                        passed = true;
    std::vector<ValidationIssue> issues;

    bool HasErrors()   const;
    bool HasWarnings() const;
    size_t ErrorCount()   const;
    size_t WarningCount() const;
    std::string Summary()    const;
};

// ──────────────────────────────────────────────────────────────
// Validation rule
// ──────────────────────────────────────────────────────────────

struct PCGValidationRule {
    uint32_t    id       = 0;
    std::string name;
    std::string category;
    bool        enabled  = true;
};

// ──────────────────────────────────────────────────────────────
// PCGValidator
// ──────────────────────────────────────────────────────────────

class PCGValidator {
public:
    // ── Geometry validation ──────────────────────────────────
    // Validate a mesh produced by HeightFieldMesher or MeshNodeGraph.
    // vertices: flat XYZ array, indices: triangle list, normals: flat XYZ array.
    ValidationReport ValidateMesh(const std::vector<float>& vertices,
                                  const std::vector<uint32_t>& indices,
                                  const std::vector<float>& normals = {}) const;

    // ── Heightfield validation ───────────────────────────────
    // width/depth are grid dimensions, data is flat float array of size w*d.
    ValidationReport ValidateHeightField(int32_t width, int32_t depth,
                                          const std::vector<float>& data) const;

    // ── Texture validation ───────────────────────────────────
    // pixels: packed RGBA8 bytes, size must equal w*h*4.
    ValidationReport ValidateTexture(int32_t width, int32_t height,
                                      const std::vector<uint8_t>& rgba8) const;

    // ── Audio validation ─────────────────────────────────────
    ValidationReport ValidateAudio(int32_t sampleRate,
                                    const std::vector<int16_t>& samples) const;

    // ── Quest validation ─────────────────────────────────────
    // Forward-declared via string checks to avoid header coupling.
    ValidationReport ValidateQuestTitle(const std::string& title,
                                         int32_t level,
                                         const std::string& locationID) const;

    // ── Custom rule registry ─────────────────────────────────
    // Register a custom lambda-based rule.
    using CheckFn = std::function<bool(ValidationReport&)>;
    uint32_t RegisterRule(const std::string& name,
                           const std::string& category,
                           CheckFn fn);
    void     EnableRule(uint32_t id, bool enabled);
    void     RunCustomRules(ValidationReport& report) const;

    // ── Batch validation ─────────────────────────────────────
    // Merge several reports into one.
    static ValidationReport Merge(const std::vector<ValidationReport>& reports);

private:
    struct RuleEntry {
        PCGValidationRule meta;
        CheckFn           fn;
    };
    uint32_t             m_nextID = 1;
    std::vector<RuleEntry> m_rules;

    static void AddIssue(ValidationReport& rpt, ValidationSeverity sev,
                         const std::string& cat, const std::string& msg,
                         const std::string& nodeID = "");
};

} // namespace PCG
