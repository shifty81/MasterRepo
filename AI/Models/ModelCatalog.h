#pragma once
#include <string>
#include <vector>
#include <map>
#include <cstdint>

namespace AI {

enum class ModelType { LLM, Embedding, VisionLanguage, CodeCompletion };
enum class QuantType  { None, Q4_0, Q4_K_M, Q5_0, Q5_K_M, Q8_0, F16 };

struct ModelDef {
    std::string id;
    std::string displayName;
    ModelType   type       = ModelType::LLM;
    QuantType   quant      = QuantType::Q4_K_M;
    std::string fileName;
    uint64_t    paramsBillion = 0;
    uint32_t    contextLen  = 4096;
    uint32_t    embedDim    = 0;
    bool        isDefault   = false;
    bool        isAvailable = false;
    std::string description;
    std::string license;
    std::vector<std::string> tags;
};

class ModelCatalog {
public:
    static ModelCatalog& Instance();

    bool LoadFromFile(const std::string& path);
    bool SaveToFile(const std::string& path) const;

    void Register(const ModelDef& def);
    void RegisterAll(const std::vector<ModelDef>& defs);
    void LoadDefaults();

    const ModelDef* FindById(const std::string& id) const;
    const ModelDef* GetDefault(ModelType type = ModelType::LLM) const;
    std::vector<const ModelDef*> GetAll() const;
    std::vector<const ModelDef*> GetByType(ModelType type) const;
    std::vector<const ModelDef*> GetAvailable() const;

    void ScanForModels(const std::string& modelsDir);

    size_t Count() const;
    void   Clear();

private:
    ModelCatalog() = default;
    std::map<std::string, ModelDef> m_models;
};

} // namespace AI
