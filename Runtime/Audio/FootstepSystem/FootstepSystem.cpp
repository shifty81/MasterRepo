#include "Runtime/Audio/FootstepSystem/FootstepSystem.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace Runtime {

struct FootstepSystem::Impl {
    std::unordered_map<std::string, SurfaceAudioDesc> surfaces;

    struct ActorRecord {
        uint32_t             handle{0};
        ActorFootstepState   state;
        float                position[3]{};
        float                lastPosition[3]{};
        float                velocity[3]{};
        bool                 grounded{true};
        std::string          surface;
        bool                 firstFrame{true};
    };

    uint32_t nextHandle{1};
    std::unordered_map<uint32_t, ActorRecord> actors;

    StepCb    onStep;
    LandingCb onLanding;
    JumpCb    onJump;

    float RandomPitch(float variance) {
        float r = (float)(std::rand() % 1000) / 1000.f - 0.5f;
        return 1.f + r * variance * 2.f;
    }

    float Speed(const float vel[3]) {
        return std::sqrt(vel[0]*vel[0]+vel[1]*vel[1]+vel[2]*vel[2]);
    }

    SurfaceAudioDesc ResolveSurface(const std::string& tag) const {
        auto it = surfaces.find(tag);
        if (it != surfaces.end()) return it->second;
        // fallback to default
        auto def = surfaces.find("default");
        return def != surfaces.end() ? def->second : SurfaceAudioDesc{};
    }
};

FootstepSystem::FootstepSystem()  : m_impl(new Impl) {}
FootstepSystem::~FootstepSystem() { Shutdown(); delete m_impl; }

void FootstepSystem::Init()     {}
void FootstepSystem::Shutdown() { m_impl->actors.clear(); }

void FootstepSystem::RegisterSurface(const std::string& tag, const SurfaceAudioDesc& desc)
{
    m_impl->surfaces[tag] = desc;
}
void FootstepSystem::UnregisterSurface(const std::string& tag)
{
    m_impl->surfaces.erase(tag);
}

uint32_t FootstepSystem::RegisterActor(const ActorFootstepDesc& desc)
{
    uint32_t h = m_impl->nextHandle++;
    Impl::ActorRecord rec;
    rec.handle = h;
    rec.state.handle = h;
    rec.state.desc = desc;
    rec.surface = desc.defaultSurface;
    m_impl->actors[h] = rec;
    return h;
}

void FootstepSystem::UnregisterActor(uint32_t handle)
{
    m_impl->actors.erase(handle);
}

void FootstepSystem::UpdateActor(uint32_t handle, const float position[3],
                                  const float velocity[3], const std::string& surfaceTag,
                                  bool isGrounded)
{
    auto it = m_impl->actors.find(handle);
    if (it == m_impl->actors.end()) return;
    auto& rec = it->second;

    if (!rec.firstFrame) {
        rec.velocity[0]=velocity[0]; rec.velocity[1]=velocity[1]; rec.velocity[2]=velocity[2];
        float speed = m_impl->Speed(velocity);
        rec.state.currentSpeed = speed;

        // Jump detection
        if (rec.grounded && !isGrounded) {
            if (m_impl->onJump) m_impl->onJump(rec.state.desc.actorId, position);
            rec.state.nextFoot = FootSide::Left;
        }

        // Landing detection
        if (!rec.grounded && isGrounded) {
            float fallH = rec.state.airTime * 9.81f * 0.5f;
            auto surf = m_impl->ResolveSurface(rec.surface);
            LandingEvent evt;
            evt.actorId = rec.state.desc.actorId;
            evt.position[0]=position[0]; evt.position[1]=position[1]; evt.position[2]=position[2];
            evt.soundCue = surf.soundCue;
            evt.fallHeight = fallH;
            evt.impactScale = std::min(1.f, fallH / 5.f);
            if (m_impl->onLanding) m_impl->onLanding(evt);
        }

        if (!isGrounded) rec.state.airTime += 0.016f; // approx, refined by Tick
        else             rec.state.airTime = 0.f;

        // Step accumulation
        if (isGrounded && speed > 0.1f) {
            float dx = position[0]-rec.lastPosition[0];
            float dz = position[2]-rec.lastPosition[2];
            float moved = std::sqrt(dx*dx + dz*dz);
            rec.state.distanceSinceLastStep += moved;

            if (rec.state.distanceSinceLastStep >= rec.state.desc.strideLength) {
                rec.state.distanceSinceLastStep = 0.f;
                auto surf = m_impl->ResolveSurface(surfaceTag);
                StepEvent evt;
                evt.actorId = rec.state.desc.actorId;
                evt.foot    = rec.state.nextFoot;
                evt.position[0]=position[0]; evt.position[1]=position[1]; evt.position[2]=position[2];
                evt.soundCue = surf.soundCue;
                evt.particleEffect = surf.particleEffect;
                float speedNorm = std::min(1.f, speed / rec.state.desc.runSpeed);
                evt.volume = surf.volumeMultiplier * (0.6f + 0.4f*speedNorm);
                evt.pitch  = m_impl->RandomPitch(surf.pitchVariance);
                evt.speed  = speed;
                rec.state.nextFoot = (rec.state.nextFoot==FootSide::Left)
                                     ? FootSide::Right : FootSide::Left;
                if (m_impl->onStep) m_impl->onStep(evt);
            }
        }
    }

    rec.lastPosition[0]=position[0]; rec.lastPosition[1]=position[1]; rec.lastPosition[2]=position[2];
    rec.grounded = isGrounded;
    rec.surface  = surfaceTag;
    rec.firstFrame = false;
}

void FootstepSystem::UpdateActorBlended(uint32_t handle, const float position[3],
                                         const float velocity[3],
                                         const std::vector<SurfaceBlend>& surfaces,
                                         bool isGrounded)
{
    std::string dominant;
    float best = 0.f;
    for (auto& s : surfaces) { if (s.weight > best) { best=s.weight; dominant=s.surface; } }
    UpdateActor(handle, position, velocity, dominant, isGrounded);
}

void FootstepSystem::Tick(float /*dt*/) {}

void FootstepSystem::SetOnStep(StepCb cb)       { m_impl->onStep = cb; }
void FootstepSystem::SetOnLanding(LandingCb cb) { m_impl->onLanding = cb; }
void FootstepSystem::SetOnJump(JumpCb cb)       { m_impl->onJump = cb; }

} // namespace Runtime
