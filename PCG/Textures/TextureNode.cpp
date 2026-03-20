#include "PCG/Textures/TextureNode.h"
#include <cmath>
#include <algorithm>
#include <fstream>
#include <stdexcept>

namespace PCG {

// ──────────────────────────────────────────────────────────────
// TextureBuffer
// ──────────────────────────────────────────────────────────────

void TextureBuffer::Resize(int32_t w, int32_t h) {
    width = w; height = h;
    pixels.assign((size_t)w * h, PixelF{});
}

PixelF TextureBuffer::Get(int32_t x, int32_t y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) return {};
    return pixels[(size_t)y * width + x];
}

void TextureBuffer::Set(int32_t x, int32_t y, PixelF p) {
    if (x < 0 || x >= width || y < 0 || y >= height) return;
    pixels[(size_t)y * width + x] = p;
}

std::vector<uint8_t> TextureBuffer::ToRGBA8() const {
    std::vector<uint8_t> out;
    out.reserve((size_t)width * height * 4);
    for (const auto& p : pixels) {
        out.push_back((uint8_t)(std::clamp(p.r, 0.f, 1.f) * 255.f));
        out.push_back((uint8_t)(std::clamp(p.g, 0.f, 1.f) * 255.f));
        out.push_back((uint8_t)(std::clamp(p.b, 0.f, 1.f) * 255.f));
        out.push_back((uint8_t)(std::clamp(p.a, 0.f, 1.f) * 255.f));
    }
    return out;
}

bool TextureBuffer::SavePPM(const std::string& path) const {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f << "P6\n" << width << " " << height << "\n255\n";
    for (const auto& p : pixels) {
        uint8_t r = (uint8_t)(std::clamp(p.r, 0.f, 1.f) * 255.f);
        uint8_t g = (uint8_t)(std::clamp(p.g, 0.f, 1.f) * 255.f);
        uint8_t b = (uint8_t)(std::clamp(p.b, 0.f, 1.f) * 255.f);
        f.put((char)r); f.put((char)g); f.put((char)b);
    }
    return true;
}

// ──────────────────────────────────────────────────────────────
// TextureGraph
// ──────────────────────────────────────────────────────────────

uint32_t TextureGraph::AddNode(TextureNode node) {
    node.id = m_nextID++;
    m_nodes.push_back(std::move(node));
    return m_nodes.back().id;
}

bool TextureGraph::Connect(uint32_t srcID, const std::string& srcPinName,
                            uint32_t dstID, const std::string& dstPinName) {
    auto* dst = GetNode(dstID);
    if (!dst) return false;
    TextureNodePin pin;
    pin.nodeID = srcID;
    pin.name   = srcPinName;
    dst->inputs.push_back(pin);
    (void)dstPinName;
    return true;
}

void TextureGraph::SetOutputNode(uint32_t id) { m_outputID = id; }

TextureNode* TextureGraph::GetNode(uint32_t id) {
    for (auto& n : m_nodes)
        if (n.id == id) return &n;
    return nullptr;
}

const TextureNode* TextureGraph::findNode(uint32_t id) const {
    for (const auto& n : m_nodes)
        if (n.id == id) return &n;
    return nullptr;
}

// ──────────────────────────────────────────────────────────────
// Noise helper — simple hash-based value noise
// ──────────────────────────────────────────────────────────────

float TextureGraph::valueNoise(float x, float y, uint64_t seed) {
    // Integer lattice coordinates
    int32_t ix = (int32_t)std::floor(x);
    int32_t iy = (int32_t)std::floor(y);
    float fx = x - (float)ix;
    float fy = y - (float)iy;

    // Smooth step
    float ux = fx * fx * (3.f - 2.f * fx);
    float uy = fy * fy * (3.f - 2.f * fy);

    auto h = [seed](int32_t xi, int32_t yi) -> float {
        uint64_t v = seed ^ ((uint64_t)(uint32_t)xi * 2654435761ULL)
                           ^ ((uint64_t)(uint32_t)yi * 2246822519ULL);
        v ^= v >> 33; v *= 0xff51afd7ed558ccdULL;
        v ^= v >> 33; v *= 0xc4ceb9fe1a85ec53ULL;
        v ^= v >> 33;
        return (float)(v & 0xFFFFFFFF) / (float)0xFFFFFFFF;
    };

    float a = h(ix,   iy);
    float b = h(ix+1, iy);
    float c = h(ix,   iy+1);
    float d = h(ix+1, iy+1);

    return a + ux*(b-a) + uy*(c-a) + ux*uy*(a-b-c+d);
}

// ──────────────────────────────────────────────────────────────
// Node evaluation
// ──────────────────────────────────────────────────────────────

TextureBuffer TextureGraph::evaluateNode(uint32_t nodeID, int32_t w, int32_t h) const {
    const TextureNode* node = findNode(nodeID);
    if (!node) { TextureBuffer b; b.Resize(w, h); return b; }

    TextureBuffer buf; buf.Resize(w, h);

    switch (node->type) {
    case TextureNodeType::Constant:
        for (auto& p : buf.pixels) p = node->color;
        break;

    case TextureNodeType::Noise:
        for (int32_t y = 0; y < h; ++y) {
            for (int32_t x = 0; x < w; ++x) {
                float nx = (float)x / w * node->frequency;
                float ny = (float)y / h * node->frequency;
                float v  = valueNoise(nx, ny, node->seed) * node->amplitude;
                buf.Set(x, y, {v, v, v, 1.f});
            }
        }
        break;

    case TextureNodeType::Gradient: {
        bool vert = (node->tileX == 0);  // repurpose tileX==0 → vertical
        for (int32_t y = 0; y < h; ++y) {
            for (int32_t x = 0; x < w; ++x) {
                float t = vert ? (float)y / (h - 1) : (float)x / (w - 1);
                PixelF p;
                p.r = node->color.r + t * (node->color2.r - node->color.r);
                p.g = node->color.g + t * (node->color2.g - node->color.g);
                p.b = node->color.b + t * (node->color2.b - node->color.b);
                p.a = 1.f;
                buf.Set(x, y, p);
            }
        }
        break;
    }

    case TextureNodeType::Blend: {
        TextureBuffer a, b2;
        if (node->inputs.size() >= 2) {
            a  = evaluateNode(node->inputs[0].nodeID, w, h);
            b2 = evaluateNode(node->inputs[1].nodeID, w, h);
        }
        float t = node->blendT;
        for (int32_t y = 0; y < h; ++y)
            for (int32_t x = 0; x < w; ++x) {
                PixelF pa = a.Get(x,y), pb = b2.Get(x,y);
                buf.Set(x, y, {pa.r+(pb.r-pa.r)*t, pa.g+(pb.g-pa.g)*t,
                               pa.b+(pb.b-pa.b)*t, 1.f});
            }
        break;
    }

    case TextureNodeType::Multiply: {
        TextureBuffer a, b2;
        if (node->inputs.size() >= 2) {
            a  = evaluateNode(node->inputs[0].nodeID, w, h);
            b2 = evaluateNode(node->inputs[1].nodeID, w, h);
        }
        for (int32_t y = 0; y < h; ++y)
            for (int32_t x = 0; x < w; ++x) {
                PixelF pa = a.Get(x,y), pb = b2.Get(x,y);
                buf.Set(x, y, {pa.r*pb.r, pa.g*pb.g, pa.b*pb.b, 1.f});
            }
        break;
    }

    case TextureNodeType::Add: {
        TextureBuffer a, b2;
        if (node->inputs.size() >= 2) {
            a  = evaluateNode(node->inputs[0].nodeID, w, h);
            b2 = evaluateNode(node->inputs[1].nodeID, w, h);
        }
        for (int32_t y = 0; y < h; ++y)
            for (int32_t x = 0; x < w; ++x) {
                PixelF pa = a.Get(x,y), pb = b2.Get(x,y);
                buf.Set(x, y, {std::min(pa.r+pb.r,1.f), std::min(pa.g+pb.g,1.f),
                               std::min(pa.b+pb.b,1.f), 1.f});
            }
        break;
    }

    case TextureNodeType::Invert: {
        TextureBuffer src;
        if (!node->inputs.empty()) src = evaluateNode(node->inputs[0].nodeID, w, h);
        for (int32_t y = 0; y < h; ++y)
            for (int32_t x = 0; x < w; ++x) {
                PixelF p = src.Get(x,y);
                buf.Set(x, y, {1.f-p.r, 1.f-p.g, 1.f-p.b, 1.f});
            }
        break;
    }

    case TextureNodeType::Threshold: {
        TextureBuffer src;
        if (!node->inputs.empty()) src = evaluateNode(node->inputs[0].nodeID, w, h);
        float cut = node->threshold;
        for (int32_t y = 0; y < h; ++y)
            for (int32_t x = 0; x < w; ++x) {
                PixelF p = src.Get(x,y);
                float v = (p.r + p.g + p.b) / 3.f >= cut ? 1.f : 0.f;
                buf.Set(x, y, {v,v,v,1.f});
            }
        break;
    }

    case TextureNodeType::Tile: {
        TextureBuffer src;
        if (!node->inputs.empty()) src = evaluateNode(node->inputs[0].nodeID, w, h);
        for (int32_t y = 0; y < h; ++y)
            for (int32_t x = 0; x < w; ++x) {
                int32_t sx = (x * node->tileX) % w;
                int32_t sy = (y * node->tileY) % h;
                buf.Set(x, y, src.Get(sx, sy));
            }
        break;
    }

    case TextureNodeType::Output:
        if (!node->inputs.empty())
            buf = evaluateNode(node->inputs[0].nodeID, w, h);
        break;
    }

    return buf;
}

TextureBuffer TextureGraph::Evaluate(int32_t width, int32_t height) const {
    if (m_outputID == 0) { TextureBuffer b; b.Resize(width, height); return b; }
    return evaluateNode(m_outputID, width, height);
}

// ──────────────────────────────────────────────────────────────
// Static convenience generators
// ──────────────────────────────────────────────────────────────

TextureBuffer TextureGraph::GenerateNoise(int32_t w, int32_t h, uint64_t seed,
                                          float frequency, float amplitude) {
    TextureBuffer buf; buf.Resize(w, h);
    for (int32_t y = 0; y < h; ++y)
        for (int32_t x = 0; x < w; ++x) {
            float v = valueNoise((float)x/w*frequency, (float)y/h*frequency, seed) * amplitude;
            buf.Set(x, y, {v,v,v,1.f});
        }
    return buf;
}

TextureBuffer TextureGraph::GenerateGradient(int32_t w, int32_t h,
                                              PixelF start, PixelF end, bool vertical) {
    TextureBuffer buf; buf.Resize(w, h);
    for (int32_t y = 0; y < h; ++y)
        for (int32_t x = 0; x < w; ++x) {
            float t = vertical ? (float)y/(h-1) : (float)x/(w-1);
            buf.Set(x, y, {start.r+(end.r-start.r)*t, start.g+(end.g-start.g)*t,
                           start.b+(end.b-start.b)*t, 1.f});
        }
    return buf;
}

TextureBuffer TextureGraph::GenerateCheckerboard(int32_t w, int32_t h,
                                                  int32_t tilesX, int32_t tilesY,
                                                  PixelF colorA, PixelF colorB) {
    TextureBuffer buf; buf.Resize(w, h);
    for (int32_t y = 0; y < h; ++y)
        for (int32_t x = 0; x < w; ++x) {
            int32_t cx = x * tilesX / w;
            int32_t cy = y * tilesY / h;
            buf.Set(x, y, ((cx + cy) % 2 == 0) ? colorA : colorB);
        }
    return buf;
}

TextureBuffer TextureGraph::GenerateVoronoi(int32_t w, int32_t h, uint64_t seed,
                                             int32_t numPoints) {
    // Generate random feature points
    std::vector<std::pair<float,float>> pts;
    pts.reserve((size_t)numPoints);
    uint64_t rng = seed;
    auto nextf = [&]() {
        rng ^= rng >> 12; rng ^= rng << 25; rng ^= rng >> 27;
        return (float)(rng & 0xFFFFFF) / (float)0xFFFFFF;
    };
    for (int i = 0; i < numPoints; ++i)
        pts.push_back({nextf(), nextf()});

    TextureBuffer buf; buf.Resize(w, h);
    for (int32_t y = 0; y < h; ++y) {
        for (int32_t x = 0; x < w; ++x) {
            float px = (float)x / w;
            float py = (float)y / h;
            float minDist = 1e9f;
            for (const auto& pt : pts) {
                float dx = px - pt.first, dy = py - pt.second;
                float d = std::sqrt(dx*dx + dy*dy);
                if (d < minDist) minDist = d;
            }
            float v = std::min(minDist * 2.f, 1.f);
            buf.Set(x, y, {v,v,v,1.f});
        }
    }
    return buf;
}

} // namespace PCG
