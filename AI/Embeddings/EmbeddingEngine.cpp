#include "AI/Embeddings/EmbeddingEngine.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <chrono>

namespace AI {

static uint64_t EmbNowMs() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
}

NullEmbeddingBackend::NullEmbeddingBackend(size_t dim) : m_dim(dim) {}
std::vector<float> NullEmbeddingBackend::Embed(const std::string&) const { return std::vector<float>(m_dim, 0.0f); }
size_t             NullEmbeddingBackend::Dimension() const { return m_dim; }
std::string        NullEmbeddingBackend::Name() const { return "Null"; }

TFIDFEmbeddingBackend::TFIDFEmbeddingBackend(size_t dim) : m_dim(dim) {}
std::vector<float> TFIDFEmbeddingBackend::Embed(const std::string& text) const {
    std::vector<float> vec(m_dim, 0.0f);
    for (size_t i = 0; i < text.size() && i < m_dim; ++i) {
        vec[i % m_dim] += static_cast<float>(static_cast<unsigned char>(text[i])) / 255.0f;
    }
    float norm = 0.0f;
    for (float v : vec) norm += v * v;
    norm = std::sqrt(norm);
    if (norm > 1e-8f) for (auto& v : vec) v /= norm;
    return vec;
}
size_t      TFIDFEmbeddingBackend::Dimension() const { return m_dim; }
std::string TFIDFEmbeddingBackend::Name() const { return "TFIDF"; }

void EmbeddingEngine::SetBackend(IEmbeddingBackend* backend) { m_backend = backend; }

Embedding EmbeddingEngine::Embed(uint64_t id, const std::string& text) {
    IEmbeddingBackend* b = m_backend ? m_backend : static_cast<IEmbeddingBackend*>(&m_nullBackend);
    Embedding emb;
    emb.id = id != 0 ? id : m_nextId++;
    emb.text = text;
    emb.dims = b->Embed(text);
    emb.timestamp = EmbNowMs();
    return emb;
}

float EmbeddingEngine::CosineSimilarity(const Embedding& a, const Embedding& b) const {
    if (a.dims.size() != b.dims.size() || a.dims.empty()) return 0.0f;
    float dot = 0.0f, na = 0.0f, nb = 0.0f;
    for (size_t i = 0; i < a.dims.size(); ++i) {
        dot += a.dims[i] * b.dims[i];
        na  += a.dims[i] * a.dims[i];
        nb  += b.dims[i] * b.dims[i];
    }
    float denom = std::sqrt(na) * std::sqrt(nb);
    return denom > 1e-8f ? dot / denom : 0.0f;
}

std::vector<std::pair<uint64_t, float>> EmbeddingEngine::FindNearest(const std::string& query, size_t topK) const {
    const IEmbeddingBackend* b = m_backend ? m_backend : static_cast<const IEmbeddingBackend*>(&m_nullBackend);
    Embedding qEmb;
    qEmb.id = 0;
    qEmb.text = query;
    qEmb.dims = b->Embed(query);
    qEmb.timestamp = 0;

    std::vector<std::pair<uint64_t, float>> results;
    for (const auto& [id, emb] : m_store) {
        float sim = CosineSimilarity(qEmb, emb);
        results.emplace_back(id, sim);
    }
    std::sort(results.begin(), results.end(),
        [](const auto& a, const auto& b){ return a.second > b.second; });
    if (results.size() > topK) results.resize(topK);
    return results;
}

void   EmbeddingEngine::Store(const Embedding& emb) { m_store[emb.id] = emb; }
void   EmbeddingEngine::Remove(uint64_t id) { m_store.erase(id); }
size_t EmbeddingEngine::Count() const { return m_store.size(); }
void   EmbeddingEngine::Clear() { m_store.clear(); }

} // namespace AI
