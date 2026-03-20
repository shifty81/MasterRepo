#include "PCG/Validation/PCGValidator.h"
#include <cmath>
#include <sstream>
#include <algorithm>

namespace PCG {

// ──────────────────────────────────────────────────────────────
// ValidationReport helpers
// ──────────────────────────────────────────────────────────────

bool ValidationReport::HasErrors() const {
    return std::any_of(issues.begin(), issues.end(),
        [](const ValidationIssue& i){ return i.severity == ValidationSeverity::Error; });
}

bool ValidationReport::HasWarnings() const {
    return std::any_of(issues.begin(), issues.end(),
        [](const ValidationIssue& i){ return i.severity == ValidationSeverity::Warning; });
}

size_t ValidationReport::ErrorCount() const {
    return (size_t)std::count_if(issues.begin(), issues.end(),
        [](const ValidationIssue& i){ return i.severity == ValidationSeverity::Error; });
}

size_t ValidationReport::WarningCount() const {
    return (size_t)std::count_if(issues.begin(), issues.end(),
        [](const ValidationIssue& i){ return i.severity == ValidationSeverity::Warning; });
}

std::string ValidationReport::Summary() const {
    std::ostringstream os;
    os << (passed ? "PASS" : "FAIL")
       << " | errors=" << ErrorCount()
       << " warnings=" << WarningCount();
    for (const auto& iss : issues)
        os << "\n  [" << (iss.severity==ValidationSeverity::Error?"ERR":
                          iss.severity==ValidationSeverity::Warning?"WARN":"INFO")
           << "] " << iss.category << ": " << iss.message;
    return os.str();
}

// ──────────────────────────────────────────────────────────────
// Private helper
// ──────────────────────────────────────────────────────────────

void PCGValidator::AddIssue(ValidationReport& rpt, ValidationSeverity sev,
                              const std::string& cat, const std::string& msg,
                              const std::string& nodeID) {
    rpt.issues.push_back({sev, cat, msg, nodeID});
    if (sev == ValidationSeverity::Error) rpt.passed = false;
}

// ──────────────────────────────────────────────────────────────
// Geometry
// ──────────────────────────────────────────────────────────────

ValidationReport PCGValidator::ValidateMesh(const std::vector<float>& vertices,
                                             const std::vector<uint32_t>& indices,
                                             const std::vector<float>& normals) const {
    ValidationReport rpt;

    if (vertices.empty()) {
        AddIssue(rpt, ValidationSeverity::Error, "geometry", "Mesh has no vertices");
        return rpt;
    }
    if (vertices.size() % 3 != 0)
        AddIssue(rpt, ValidationSeverity::Error, "geometry",
                 "Vertex array size not multiple of 3");

    if (indices.empty()) {
        AddIssue(rpt, ValidationSeverity::Warning, "geometry", "Mesh has no indices");
    } else {
        if (indices.size() % 3 != 0)
            AddIssue(rpt, ValidationSeverity::Error, "geometry",
                     "Index count not multiple of 3 (not a triangle list)");

        uint32_t maxIdx = (uint32_t)(vertices.size() / 3);
        for (size_t i = 0; i < indices.size(); ++i) {
            if (indices[i] >= maxIdx) {
                AddIssue(rpt, ValidationSeverity::Error, "geometry",
                         "Index out of range at position " + std::to_string(i));
                break; // one report is enough
            }
        }
    }

    // Check for NaN/Inf in vertices
    for (size_t i = 0; i < vertices.size(); ++i) {
        if (!std::isfinite(vertices[i])) {
            AddIssue(rpt, ValidationSeverity::Error, "geometry",
                     "Non-finite vertex value at component " + std::to_string(i));
            break;
        }
    }

    // Normals size check
    if (!normals.empty() && normals.size() != vertices.size())
        AddIssue(rpt, ValidationSeverity::Warning, "geometry",
                 "Normal array size does not match vertex array size");

    return rpt;
}

// ──────────────────────────────────────────────────────────────
// HeightField
// ──────────────────────────────────────────────────────────────

ValidationReport PCGValidator::ValidateHeightField(int32_t width, int32_t depth,
                                                    const std::vector<float>& data) const {
    ValidationReport rpt;

    if (width <= 0 || depth <= 0) {
        AddIssue(rpt, ValidationSeverity::Error, "heightfield",
                 "Dimensions must be > 0 (got " + std::to_string(width)
                 + "x" + std::to_string(depth) + ")");
        return rpt;
    }

    size_t expected = (size_t)width * depth;
    if (data.size() != expected)
        AddIssue(rpt, ValidationSeverity::Error, "heightfield",
                 "Data size mismatch: expected " + std::to_string(expected)
                 + ", got " + std::to_string(data.size()));

    float minH = 1e9f, maxH = -1e9f;
    for (float v : data) {
        if (!std::isfinite(v)) {
            AddIssue(rpt, ValidationSeverity::Error, "heightfield",
                     "Non-finite height value in data");
            break;
        }
        minH = std::min(minH, v);
        maxH = std::max(maxH, v);
    }

    if (!data.empty() && (maxH - minH) < 1e-6f)
        AddIssue(rpt, ValidationSeverity::Warning, "heightfield",
                 "Height range is nearly zero — terrain will be flat");

    return rpt;
}

// ──────────────────────────────────────────────────────────────
// Texture
// ──────────────────────────────────────────────────────────────

ValidationReport PCGValidator::ValidateTexture(int32_t width, int32_t height,
                                                const std::vector<uint8_t>& rgba8) const {
    ValidationReport rpt;

    if (width <= 0 || height <= 0) {
        AddIssue(rpt, ValidationSeverity::Error, "texture",
                 "Texture dimensions must be > 0");
        return rpt;
    }

    size_t expected = (size_t)width * height * 4;
    if (rgba8.size() != expected)
        AddIssue(rpt, ValidationSeverity::Error, "texture",
                 "Pixel data size mismatch: expected " + std::to_string(expected)
                 + ", got " + std::to_string(rgba8.size()));

    // Warn if power-of-two mismatch (GPU recommendation)
    auto isPow2 = [](int32_t v){ return v > 0 && (v & (v-1)) == 0; };
    if (!isPow2(width) || !isPow2(height))
        AddIssue(rpt, ValidationSeverity::Warning, "texture",
                 "Texture dimensions are not power-of-two (" + std::to_string(width)
                 + "x" + std::to_string(height) + ")");

    return rpt;
}

// ──────────────────────────────────────────────────────────────
// Audio
// ──────────────────────────────────────────────────────────────

ValidationReport PCGValidator::ValidateAudio(int32_t sampleRate,
                                              const std::vector<int16_t>& samples) const {
    ValidationReport rpt;

    if (sampleRate <= 0) {
        AddIssue(rpt, ValidationSeverity::Error, "audio",
                 "Sample rate must be > 0");
        return rpt;
    }
    if (samples.empty()) {
        AddIssue(rpt, ValidationSeverity::Error, "audio", "No audio samples");
        return rpt;
    }

    float duration = (float)samples.size() / (float)sampleRate;
    if (duration < 0.01f)
        AddIssue(rpt, ValidationSeverity::Warning, "audio",
                 "Audio clip is very short (" + std::to_string(duration) + "s)");
    if (duration > 300.f)
        AddIssue(rpt, ValidationSeverity::Warning, "audio",
                 "Audio clip is very long (" + std::to_string(duration) + "s)");

    // Check peak amplitude (warn if near zero or clipping)
    int16_t maxAbs = 0;
    for (int16_t s : samples) {
        int16_t a = s < 0 ? (int16_t)-s : s;
        if (a > maxAbs) maxAbs = a;
    }
    if (maxAbs < 100)
        AddIssue(rpt, ValidationSeverity::Warning, "audio",
                 "Audio peak amplitude is very low (near silence)");
    if (maxAbs == 32767)
        AddIssue(rpt, ValidationSeverity::Warning, "audio",
                 "Audio may be clipping (peak at max int16)");

    return rpt;
}

// ──────────────────────────────────────────────────────────────
// Quest
// ──────────────────────────────────────────────────────────────

ValidationReport PCGValidator::ValidateQuestTitle(const std::string& title,
                                                    int32_t level,
                                                    const std::string& locationID) const {
    ValidationReport rpt;
    if (title.empty())
        AddIssue(rpt, ValidationSeverity::Error, "quest", "Quest title is empty");
    if (level < 1 || level > 100)
        AddIssue(rpt, ValidationSeverity::Warning, "quest",
                 "Quest level out of range: " + std::to_string(level));
    if (locationID.empty())
        AddIssue(rpt, ValidationSeverity::Warning, "quest", "Quest has no location ID");
    return rpt;
}

// ──────────────────────────────────────────────────────────────
// Custom rules
// ──────────────────────────────────────────────────────────────

uint32_t PCGValidator::RegisterRule(const std::string& name,
                                     const std::string& category,
                                     CheckFn fn) {
    RuleEntry entry;
    entry.meta.id       = m_nextID++;
    entry.meta.name     = name;
    entry.meta.category = category;
    entry.fn            = std::move(fn);
    m_rules.push_back(std::move(entry));
    return m_rules.back().meta.id;
}

void PCGValidator::EnableRule(uint32_t id, bool enabled) {
    for (auto& r : m_rules)
        if (r.meta.id == id) { r.meta.enabled = enabled; return; }
}

void PCGValidator::RunCustomRules(ValidationReport& report) const {
    for (const auto& r : m_rules) {
        if (!r.meta.enabled) continue;
        r.fn(report);
    }
}

ValidationReport PCGValidator::Merge(const std::vector<ValidationReport>& reports) {
    ValidationReport out;
    for (const auto& r : reports) {
        for (const auto& iss : r.issues)
            out.issues.push_back(iss);
        if (!r.passed) out.passed = false;
    }
    return out;
}

} // namespace PCG
