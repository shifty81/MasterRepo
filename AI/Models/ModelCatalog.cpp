#include "AI/Models/ModelCatalog.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace AI {

// ── helpers ──────────────────────────────────────────────────────────────────

static std::string modelTypeToStr(ModelType t) {
    switch (t) {
        case ModelType::LLM:             return "LLM";
        case ModelType::Embedding:       return "Embedding";
        case ModelType::VisionLanguage:  return "VisionLanguage";
        case ModelType::CodeCompletion:  return "CodeCompletion";
    }
    return "LLM";
}

static ModelType strToModelType(const std::string& s) {
    if (s == "Embedding")      return ModelType::Embedding;
    if (s == "VisionLanguage") return ModelType::VisionLanguage;
    if (s == "CodeCompletion") return ModelType::CodeCompletion;
    return ModelType::LLM;
}

static std::string quantTypeToStr(QuantType q) {
    switch (q) {
        case QuantType::None:   return "None";
        case QuantType::Q4_0:   return "Q4_0";
        case QuantType::Q4_K_M: return "Q4_K_M";
        case QuantType::Q5_0:   return "Q5_0";
        case QuantType::Q5_K_M: return "Q5_K_M";
        case QuantType::Q8_0:   return "Q8_0";
        case QuantType::F16:    return "F16";
    }
    return "None";
}

static QuantType strToQuantType(const std::string& s) {
    if (s == "Q4_0")   return QuantType::Q4_0;
    if (s == "Q4_K_M") return QuantType::Q4_K_M;
    if (s == "Q5_0")   return QuantType::Q5_0;
    if (s == "Q5_K_M") return QuantType::Q5_K_M;
    if (s == "Q8_0")   return QuantType::Q8_0;
    if (s == "F16")    return QuantType::F16;
    return QuantType::None;
}

// Very small hand-rolled JSON helpers (no external deps).
static std::string jsonStr(const std::string& s) {
    std::string out = "\"";
    for (char c : s) {
        if (c == '"')  out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else           out += c;
    }
    out += '"';
    return out;
}

static std::string jsonBool(bool b)  { return b ? "true" : "false"; }
static std::string jsonUint(uint64_t v) { return std::to_string(v); }

// Minimal JSON value extractor – not a full parser, covers flat objects only.
static std::string extractJsonString(const std::string& json, const std::string& key) {
    std::string pat = "\"" + key + "\"";
    auto pos = json.find(pat);
    if (pos == std::string::npos) return {};
    pos = json.find(':', pos);
    if (pos == std::string::npos) return {};
    pos = json.find('"', pos);
    if (pos == std::string::npos) return {};
    ++pos;
    std::string val;
    while (pos < json.size() && json[pos] != '"') {
        if (json[pos] == '\\' && pos + 1 < json.size()) { ++pos; }
        val += json[pos++];
    }
    return val;
}

static bool extractJsonBool(const std::string& json, const std::string& key) {
    std::string pat = "\"" + key + "\"";
    auto pos = json.find(pat);
    if (pos == std::string::npos) return false;
    pos = json.find(':', pos);
    if (pos == std::string::npos) return false;
    while (pos < json.size() && (json[pos] == ':' || json[pos] == ' ')) ++pos;
    return json.substr(pos, 4) == "true";
}

static uint64_t extractJsonUint(const std::string& json, const std::string& key) {
    std::string pat = "\"" + key + "\"";
    auto pos = json.find(pat);
    if (pos == std::string::npos) return 0;
    pos = json.find(':', pos);
    if (pos == std::string::npos) return 0;
    while (pos < json.size() && (json[pos] == ':' || json[pos] == ' ')) ++pos;
    uint64_t v = 0;
    while (pos < json.size() && json[pos] >= '0' && json[pos] <= '9')
        v = v * 10 + (json[pos++] - '0');
    return v;
}

static std::vector<std::string> extractJsonStringArray(const std::string& json,
                                                        const std::string& key) {
    std::vector<std::string> result;
    std::string pat = "\"" + key + "\"";
    auto pos = json.find(pat);
    if (pos == std::string::npos) return result;
    pos = json.find('[', pos);
    if (pos == std::string::npos) return result;
    ++pos;
    while (pos < json.size() && json[pos] != ']') {
        pos = json.find('"', pos);
        if (pos == std::string::npos || json[pos - 1] == ']') break;
        ++pos;
        std::string val;
        while (pos < json.size() && json[pos] != '"') val += json[pos++];
        result.push_back(val);
        ++pos;
    }
    return result;
}

// ── singleton ─────────────────────────────────────────────────────────────────

ModelCatalog& ModelCatalog::Instance() {
    static ModelCatalog s;
    return s;
}

// ── registration ──────────────────────────────────────────────────────────────

void ModelCatalog::Register(const ModelDef& def) {
    m_models[def.id] = def;
}

void ModelCatalog::RegisterAll(const std::vector<ModelDef>& defs) {
    for (auto& d : defs) Register(d);
}

void ModelCatalog::LoadDefaults() {
    {   ModelDef d;
        d.id = "llama3"; d.displayName = "LLaMA 3 8B";
        d.type = ModelType::LLM; d.quant = QuantType::Q4_K_M;
        d.fileName = "llama3-8b-instruct-q4_k_m.gguf";
        d.paramsBillion = 8; d.contextLen = 8192; d.embedDim = 0;
        d.isDefault = true; d.isAvailable = false;
        d.description = "Meta's LLaMA 3 8B Instruct.";
        d.license = "Meta Community License";
        d.tags = {"chat", "instruct", "general"};
        Register(d); }

    {   ModelDef d;
        d.id = "mistral"; d.displayName = "Mistral 7B";
        d.type = ModelType::LLM; d.quant = QuantType::Q4_K_M;
        d.fileName = "mistral-7b-instruct-q4_k_m.gguf";
        d.paramsBillion = 7; d.contextLen = 32768; d.embedDim = 0;
        d.isDefault = false; d.isAvailable = false;
        d.description = "Mistral 7B Instruct.";
        d.license = "Apache 2.0";
        d.tags = {"chat", "instruct"};
        Register(d); }

    {   ModelDef d;
        d.id = "codellama"; d.displayName = "CodeLlama 13B";
        d.type = ModelType::CodeCompletion; d.quant = QuantType::Q4_K_M;
        d.fileName = "codellama-13b-q4_k_m.gguf";
        d.paramsBillion = 13; d.contextLen = 16384; d.embedDim = 0;
        d.isDefault = false; d.isAvailable = false;
        d.description = "Meta's CodeLlama 13B for code completion.";
        d.license = "Meta Community License";
        d.tags = {"code", "instruct"};
        Register(d); }

    {   ModelDef d;
        d.id = "orca-mini"; d.displayName = "Orca Mini 3B";
        d.type = ModelType::LLM; d.quant = QuantType::Q4_0;
        d.fileName = "orca-mini-3b-q4_0.gguf";
        d.paramsBillion = 3; d.contextLen = 2048; d.embedDim = 0;
        d.isDefault = false; d.isAvailable = false;
        d.description = "Small and fast Orca Mini 3B.";
        d.license = "CC-BY-NC-4.0";
        d.tags = {"small", "fast", "chat"};
        Register(d); }

    {   ModelDef d;
        d.id = "nomic-embed"; d.displayName = "Nomic Embed";
        d.type = ModelType::Embedding; d.quant = QuantType::Q8_0;
        d.fileName = "nomic-embed-text-v1.5-q8_0.gguf";
        d.paramsBillion = 0; d.contextLen = 8192; d.embedDim = 768;
        d.isDefault = false; d.isAvailable = false;
        d.description = "Nomic text embedding model.";
        d.license = "Apache 2.0";
        d.tags = {"embeddings"};
        Register(d); }
}

// ── queries ───────────────────────────────────────────────────────────────────

const ModelDef* ModelCatalog::FindById(const std::string& id) const {
    auto it = m_models.find(id);
    return it != m_models.end() ? &it->second : nullptr;
}

const ModelDef* ModelCatalog::GetDefault(ModelType type) const {
    for (auto& [id, def] : m_models)
        if (def.type == type && def.isDefault) return &def;
    for (auto& [id, def] : m_models)
        if (def.type == type) return &def;
    return nullptr;
}

std::vector<const ModelDef*> ModelCatalog::GetAll() const {
    std::vector<const ModelDef*> out;
    out.reserve(m_models.size());
    for (auto& [id, def] : m_models) out.push_back(&def);
    return out;
}

std::vector<const ModelDef*> ModelCatalog::GetByType(ModelType type) const {
    std::vector<const ModelDef*> out;
    for (auto& [id, def] : m_models)
        if (def.type == type) out.push_back(&def);
    return out;
}

std::vector<const ModelDef*> ModelCatalog::GetAvailable() const {
    std::vector<const ModelDef*> out;
    for (auto& [id, def] : m_models)
        if (def.isAvailable) out.push_back(&def);
    return out;
}

size_t ModelCatalog::Count() const { return m_models.size(); }
void   ModelCatalog::Clear()       { m_models.clear(); }

// ── filesystem scan ──────────────────────────────────────────────────────────

void ModelCatalog::ScanForModels(const std::string& modelsDir) {
    namespace fs = std::filesystem;
    if (!fs::exists(modelsDir)) return;
    for (auto& entry : fs::directory_iterator(modelsDir)) {
        if (entry.path().extension() != ".gguf") continue;
        std::string fname = entry.path().filename().string();
        for (auto& [id, def] : m_models) {
            if (def.fileName == fname) {
                def.isAvailable = true;
                break;
            }
        }
    }
}

// ── JSON I/O ─────────────────────────────────────────────────────────────────

bool ModelCatalog::LoadFromFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) return false;
    std::ostringstream ss;
    ss << f.rdbuf();
    std::string json = ss.str();

    // Parse array of model objects.  Each object is delimited by '{' ... '}'.
    size_t pos = 0;
    while ((pos = json.find('{', pos)) != std::string::npos) {
        size_t end = json.find('}', pos);
        if (end == std::string::npos) break;
        std::string obj = json.substr(pos, end - pos + 1);
        pos = end + 1;

        ModelDef d;
        d.id           = extractJsonString(obj, "id");
        if (d.id.empty()) continue;
        d.displayName  = extractJsonString(obj, "displayName");
        d.type         = strToModelType(extractJsonString(obj, "type"));
        d.quant        = strToQuantType(extractJsonString(obj, "quant"));
        d.fileName     = extractJsonString(obj, "fileName");
        d.paramsBillion= static_cast<uint64_t>(extractJsonUint(obj, "paramsBillion"));
        d.contextLen   = static_cast<uint32_t>(extractJsonUint(obj, "contextLen"));
        d.embedDim     = static_cast<uint32_t>(extractJsonUint(obj, "embedDim"));
        d.isDefault    = extractJsonBool(obj, "isDefault");
        d.isAvailable  = extractJsonBool(obj, "isAvailable");
        d.description  = extractJsonString(obj, "description");
        d.license      = extractJsonString(obj, "license");
        d.tags         = extractJsonStringArray(obj, "tags");
        Register(d);
    }
    return true;
}

bool ModelCatalog::SaveToFile(const std::string& path) const {
    std::ofstream f(path);
    if (!f) return false;

    f << "[\n";
    bool first = true;
    for (auto& [id, d] : m_models) {
        if (!first) f << ",\n";
        first = false;
        f << "  {\n";
        f << "    \"id\": "            << jsonStr(d.id)                         << ",\n";
        f << "    \"displayName\": "   << jsonStr(d.displayName)                << ",\n";
        f << "    \"type\": "          << jsonStr(modelTypeToStr(d.type))       << ",\n";
        f << "    \"quant\": "         << jsonStr(quantTypeToStr(d.quant))      << ",\n";
        f << "    \"fileName\": "      << jsonStr(d.fileName)                   << ",\n";
        f << "    \"paramsBillion\": " << jsonUint(d.paramsBillion)             << ",\n";
        f << "    \"contextLen\": "    << jsonUint(d.contextLen)                << ",\n";
        f << "    \"embedDim\": "      << jsonUint(d.embedDim)                  << ",\n";
        f << "    \"isDefault\": "     << jsonBool(d.isDefault)                 << ",\n";
        f << "    \"isAvailable\": "   << jsonBool(d.isAvailable)               << ",\n";
        f << "    \"description\": "   << jsonStr(d.description)                << ",\n";
        f << "    \"license\": "       << jsonStr(d.license)                    << ",\n";
        f << "    \"tags\": [";
        for (size_t i = 0; i < d.tags.size(); ++i) {
            if (i) f << ", ";
            f << jsonStr(d.tags[i]);
        }
        f << "]\n  }";
    }
    f << "\n]\n";
    return f.good();
}

} // namespace AI
