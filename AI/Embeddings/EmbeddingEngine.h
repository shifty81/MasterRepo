#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace AI {

struct Embedding {
    uint64_t           id        = 0;
    std::string        text;
    std::vector<float> dims;
    uint64_t           timestamp = 0;
};

class IEmbeddingBackend {
public:
    virtual ~IEmbeddingBackend() = default;
    virtual std::vector<float> Embed(const std::string& text) const = 0;
    virtual size_t             Dimension() const = 0;
    virtual std::string        Name() const = 0;
};

class NullEmbeddingBackend : public IEmbeddingBackend {
public:
    explicit NullEmbeddingBackend(size_t dim = 128);
    std::vector<float> Embed(const std::string& text) const override;
    size_t             Dimension() const override;
    std::string        Name() const override;
private:
    size_t m_dim;
};

class TFIDFEmbeddingBackend : public IEmbeddingBackend {
public:
    explicit TFIDFEmbeddingBackend(size_t dim = 256);
    std::vector<float> Embed(const std::string& text) const override;
    size_t             Dimension() const override;
    std::string        Name() const override;
private:
    size_t m_dim;
};

class EmbeddingEngine {
public:
    void SetBackend(IEmbeddingBackend* backend);

    Embedding Embed(uint64_t id, const std::string& text);
    float     CosineSimilarity(const Embedding& a, const Embedding& b) const;
    std::vector<std::pair<uint64_t, float>> FindNearest(const std::string& query, size_t topK) const;

    void   Store(const Embedding& emb);
    void   Remove(uint64_t id);
    size_t Count() const;
    void   Clear();

private:
    IEmbeddingBackend*                      m_backend = nullptr;
    NullEmbeddingBackend                    m_nullBackend;
    std::unordered_map<uint64_t, Embedding> m_store;
    uint64_t                                m_nextId = 1;
};

} // namespace AI
