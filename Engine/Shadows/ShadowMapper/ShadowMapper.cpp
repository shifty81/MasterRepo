#include "Engine/Shadows/ShadowMapper/ShadowMapper.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace Engine {

// ── Mat4 helpers (minimal, row-major) ─────────────────────────────────────
static Math::Mat4 mat4Identity() {
    Math::Mat4 m{};
    m.m[0][0]=m.m[1][1]=m.m[2][2]=m.m[3][3]=1.0f;
    return m;
}

static Math::Vec3 mat4MulVec3(const Math::Mat4& m, const Math::Vec3& v, float w=1.0f) {
    float x=m.m[0][0]*v.x+m.m[0][1]*v.y+m.m[0][2]*v.z+m.m[0][3]*w;
    float y=m.m[1][0]*v.x+m.m[1][1]*v.y+m.m[1][2]*v.z+m.m[1][3]*w;
    float z=m.m[2][0]*v.x+m.m[2][1]*v.y+m.m[2][2]*v.z+m.m[2][3]*w;
    float ww=m.m[3][0]*v.x+m.m[3][1]*v.y+m.m[3][2]*v.z+m.m[3][3]*w;
    if (std::abs(ww)>1e-7f) { x/=ww; y/=ww; z/=ww; }
    return {x,y,z};
}

static Math::Vec3 vSub(const Math::Vec3& a, const Math::Vec3& b){ return {a.x-b.x,a.y-b.y,a.z-b.z}; }
static Math::Vec3 vAdd(const Math::Vec3& a, const Math::Vec3& b){ return {a.x+b.x,a.y+b.y,a.z+b.z}; }
static Math::Vec3 vMul(const Math::Vec3& v, float s){ return {v.x*s,v.y*s,v.z*s}; }
static float vDot(const Math::Vec3& a,const Math::Vec3& b){ return a.x*b.x+a.y*b.y+a.z*b.z; }
static float vLen(const Math::Vec3& v){ return std::sqrt(vDot(v,v)); }
static Math::Vec3 vNorm(const Math::Vec3& v){ float l=vLen(v); return l>1e-7f?vMul(v,1.0f/l):Math::Vec3{}; }
static Math::Vec3 vCross(const Math::Vec3& a,const Math::Vec3& b){
    return {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x};
}

// Build look-at matrix toward lightDir
static Math::Mat4 lightViewMatrix(const Math::Vec3& eye, const Math::Vec3& lightDir) {
    Math::Vec3 fwd = vNorm(lightDir);
    Math::Vec3 up = {0,1,0};
    if (std::abs(vDot(fwd,up)) > 0.999f) up = {1,0,0};
    Math::Vec3 right = vNorm(vCross(up, fwd));
    up = vCross(fwd, right);
    Math::Mat4 m{};
    m.m[0][0]=right.x; m.m[0][1]=right.y; m.m[0][2]=right.z; m.m[0][3]=-vDot(right,eye);
    m.m[1][0]=up.x;    m.m[1][1]=up.y;    m.m[1][2]=up.z;    m.m[1][3]=-vDot(up,eye);
    m.m[2][0]=fwd.x;   m.m[2][1]=fwd.y;   m.m[2][2]=fwd.z;   m.m[2][3]=-vDot(fwd,eye);
    m.m[3][3]=1.0f;
    return m;
}

// Build orthographic projection
static Math::Mat4 orthoMatrix(float l, float r, float b, float t, float n, float f2) {
    Math::Mat4 m{};
    m.m[0][0]= 2.0f/(r-l);  m.m[0][3]=-(r+l)/(r-l);
    m.m[1][1]= 2.0f/(t-b);  m.m[1][3]=-(t+b)/(t-b);
    m.m[2][2]=-2.0f/(f2-n); m.m[2][3]=-(f2+n)/(f2-n);
    m.m[3][3]= 1.0f;
    return m;
}

static Math::Mat4 mat4Mul(const Math::Mat4& a, const Math::Mat4& b) {
    Math::Mat4 c{};
    for (int i=0;i<4;++i) for (int j=0;j<4;++j)
        for (int k=0;k<4;++k) c.m[i][j]+=a.m[i][k]*b.m[k][j];
    return c;
}

// ── Impl ─────────────────────────────────────────────────────────────────
struct ShadowMapper::Impl {
    uint32_t numCascades{3};
    CascadeConfig cfg;
    PCFConfig pcf;
    std::unordered_map<uint32_t, ShadowMap> shadowMaps;
};

ShadowMapper::ShadowMapper() : m_impl(new Impl()) {}
ShadowMapper::ShadowMapper(uint32_t n, const CascadeConfig& cfg) : m_impl(new Impl()) {
    m_impl->numCascades = n;
    m_impl->cfg = cfg;
}
ShadowMapper::~ShadowMapper() { delete m_impl; }

void ShadowMapper::SetCascadeCount(uint32_t n) { m_impl->numCascades = n; }
uint32_t ShadowMapper::CascadeCount() const { return m_impl->numCascades; }
void ShadowMapper::SetCascadeConfig(const CascadeConfig& c) { m_impl->cfg = c; }
const CascadeConfig& ShadowMapper::GetCascadeConfig() const { return m_impl->cfg; }
void ShadowMapper::SetPCFConfig(const PCFConfig& c) { m_impl->pcf = c; }
const PCFConfig& ShadowMapper::GetPCFConfig() const { return m_impl->pcf; }

std::vector<float> ShadowMapper::ComputeSplitDistances(float near, float far) const {
    uint32_t n = m_impl->numCascades;
    float lam = m_impl->cfg.splitLambda;
    std::vector<float> splits(n + 1);
    splits[0] = near;
    for (uint32_t i = 1; i < n; ++i) {
        float iDivN = static_cast<float>(i) / static_cast<float>(n);
        float log   = near * std::pow(far / near, iDivN);
        float uni   = near + (far - near) * iDivN;
        splits[i]   = lam * log + (1.0f - lam) * uni;
    }
    splits[n] = far;
    return splits;
}

// ── Frustum corners ───────────────────────────────────────────────────────
// Inverse the NDC cube corners through invViewProj to get world space
std::array<Math::Vec3, 8> ShadowMapper::ExtractFrustumCorners(const Math::Mat4& inv) {
    static const float ndcCorners[8][3] = {
        {-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},
        {-1,-1, 1},{1,-1, 1},{1,1, 1},{-1,1, 1}
    };
    std::array<Math::Vec3,8> out;
    for (int i=0;i<8;++i)
        out[i] = mat4MulVec3(inv, {ndcCorners[i][0],ndcCorners[i][1],ndcCorners[i][2]});
    return out;
}

LightSpaceMatrix ShadowMapper::FrustumToLightSpace(
    const std::array<Math::Vec3,8>& corners,
    const Math::Vec3& lightDir,
    float depthBias,
    float nearOffset)
{
    // Centroid
    Math::Vec3 centre{};
    for (auto& c : corners) centre = vAdd(centre, vMul(c, 1.0f/8.0f));
    Math::Mat4 view = lightViewMatrix(centre, lightDir);

    // Transform corners into light space; find bounds
    float minX=1e30f,maxX=-1e30f,minY=1e30f,maxY=-1e30f,minZ=1e30f,maxZ=-1e30f;
    for (auto& c : corners) {
        Math::Vec3 lc = mat4MulVec3(view, c);
        minX=std::min(minX,lc.x); maxX=std::max(maxX,lc.x);
        minY=std::min(minY,lc.y); maxY=std::max(maxY,lc.y);
        minZ=std::min(minZ,lc.z); maxZ=std::max(maxZ,lc.z);
    }
    Math::Mat4 proj = orthoMatrix(minX,maxX,minY,maxY,minZ-nearOffset,maxZ+depthBias);
    LightSpaceMatrix ls;
    ls.view = view;
    ls.proj = proj;
    ls.viewProj = mat4Mul(proj, view);
    return ls;
}

std::vector<LightSpaceMatrix> ShadowMapper::ComputeCascades(
    const Math::Mat4& camVP,
    const Math::Vec3& lightDir,
    float near, float far) const
{
    // Invert camVP (simplified: use stored splits + sub-frustums)
    auto splits = ComputeSplitDistances(near, far);
    uint32_t n = m_impl->numCascades;
    std::vector<LightSpaceMatrix> result(n);

    // We approximate sub-frustum corners by scaling the full frustum corners
    // between near and split[i+1] proportionally.
    auto fullCorners = ExtractFrustumCorners(camVP); // crude approximation

    for (uint32_t i = 0; i < n; ++i) {
        float t0 = (splits[i]   - near) / (far - near);
        float t1 = (splits[i+1] - near) / (far - near);
        // Lerp near-face (0..3) and far-face (4..7)
        std::array<Math::Vec3,8> sub;
        for (int j=0;j<4;++j) {
            sub[j]   = vAdd(vMul(fullCorners[j],1-t0), vMul(fullCorners[j+4],t0));
            sub[j+4] = vAdd(vMul(fullCorners[j],1-t1), vMul(fullCorners[j+4],t1));
        }
        result[i] = FrustumToLightSpace(sub, lightDir,
                                         m_impl->cfg.depthBias);
        result[i].splitNear    = splits[i];
        result[i].splitFar     = splits[i+1];
        result[i].cascadeIndex = i;
    }
    return result;
}

// ── Shadow map registry ────────────────────────────────────────────────────
uint32_t ShadowMapper::RegisterLight(uint32_t lightId) {
    ShadowMap sm;
    sm.lightId          = lightId;
    sm.resolution       = m_impl->cfg.resolution;
    sm.valid            = true;
    m_impl->shadowMaps[lightId] = sm;
    return lightId;
}

void ShadowMapper::UnregisterLight(uint32_t id) { m_impl->shadowMaps.erase(id); }

const ShadowMap* ShadowMapper::GetShadowMap(uint32_t id) const {
    auto it = m_impl->shadowMaps.find(id);
    return it != m_impl->shadowMaps.end() ? &it->second : nullptr;
}
ShadowMap* ShadowMapper::GetShadowMap(uint32_t id) {
    auto it = m_impl->shadowMaps.find(id);
    return it != m_impl->shadowMaps.end() ? &it->second : nullptr;
}

std::vector<uint32_t> ShadowMapper::RegisteredLights() const {
    std::vector<uint32_t> out;
    for (const auto& [id,_] : m_impl->shadowMaps) out.push_back(id);
    return out;
}

} // namespace Engine
