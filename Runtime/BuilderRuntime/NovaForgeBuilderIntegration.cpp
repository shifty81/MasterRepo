#include "Runtime/BuilderRuntime/NovaForgeBuilderIntegration.h"

#include "Builder/Assembly/Assembly.h"
#include "Builder/Parts/PartDef.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

namespace Runtime {

// ── helpers ───────────────────────────────────────────────────────────────────

static uint64_t NowMs() {
    using namespace std::chrono;
    return (uint64_t)duration_cast<milliseconds>(
        steady_clock::now().time_since_epoch()).count();
}

/// Minimal JSON value extractor — avoids external dependencies.
static std::string JsonStr(const std::string& json,
                            const std::string& key,
                            const std::string& fallback = "") {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return fallback;
    pos = json.find(':', pos);
    if (pos == std::string::npos) return fallback;
    pos = json.find_first_not_of(" \t\r\n", pos + 1);
    if (pos == std::string::npos) return fallback;
    if (json[pos] == '"') {
        size_t start = pos + 1;
        size_t end   = json.find('"', start);
        return end == std::string::npos ? fallback : json.substr(start, end - start);
    }
    // Number / bool — read until comma/} whichever comes first
    size_t end = json.find_first_of(",}", pos);
    std::string val = json.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
    while (!val.empty() && (val.back() == ' ' || val.back() == '\t' ||
                             val.back() == '\r' || val.back() == '\n'))
        val.pop_back();
    return val;
}

static float JsonFloat(const std::string& json, const std::string& key,
                        float fallback = 0.0f) {
    std::string v = JsonStr(json, key);
    if (v.empty()) return fallback;
    try { return std::stof(v); } catch (...) { return fallback; }
}

static uint32_t JsonUint(const std::string& json, const std::string& key,
                          uint32_t fallback = 0) {
    std::string v = JsonStr(json, key);
    if (v.empty()) return fallback;
    try { return (uint32_t)std::stoul(v); } catch (...) { return fallback; }
}

/// Serialize a PlacedPart assembly to a minimal JSON string.
static std::string AssemblyToJson(const Builder::Assembly& asm_,
                                   const std::string& name,
                                   BuildTargetType type) {
    std::ostringstream js;
    js << "{\n";
    js << "  \"name\": \"" << name << "\",\n";
    js << "  \"type\": " << (int)type << ",\n";
    js << "  \"savedMs\": " << NowMs() << ",\n";
    js << "  \"partCount\": " << asm_.PartCount() << ",\n";
    js << "  \"linkCount\": " << asm_.LinkCount() << "\n";
    js << "}\n";
    return js.str();
}

// ── Impl ─────────────────────────────────────────────────────────────────────

struct NovaForgeBuilderIntegration::Impl {
    Builder::PartLibrary          partLib;
    Builder::SnapRules            snapRules;
    Builder::CollisionBuilder     collisionBuilder;

    std::unordered_map<uint32_t, std::unique_ptr<BuildSession>> sessions;
    uint32_t nextSessionId{1};
    uint32_t activeSessionId{0};

    CraftingGateFn craftingGate;
    ConsumePartFn  consumePart;
    BuilderIntegrationCallbacks callbacks;
};

// ── NovaForgeBuilderIntegration ────────────────────────────────────────────────

NovaForgeBuilderIntegration::NovaForgeBuilderIntegration() : m_impl(new Impl{}) {
    // Register default snap type compatibilities for NovaForge
    m_impl->snapRules.RegisterCompatibility("hull",    "hull");
    m_impl->snapRules.RegisterCompatibility("engine",  "hull");
    m_impl->snapRules.RegisterCompatibility("weapon",  "hull");
    m_impl->snapRules.RegisterCompatibility("shield",  "hull");
    m_impl->snapRules.RegisterCompatibility("power",   "hull");
    m_impl->snapRules.RegisterCompatibility("utility", "hull");
    m_impl->snapRules.RegisterCompatibility("armor",   "hull");
    m_impl->snapRules.RegisterCompatibility("deck",    "hull");
    m_impl->snapRules.RegisterCompatibility("generic", "generic");
}

NovaForgeBuilderIntegration::~NovaForgeBuilderIntegration() { delete m_impl; }

// ── Library loading ───────────────────────────────────────────────────────────

size_t NovaForgeBuilderIntegration::LoadPartLibrary(const std::string& partsDir) {
    size_t loaded = 0;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(partsDir, ec)) {
        if (ec) break;
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".json") continue;

        std::ifstream f(entry.path().string());
        if (!f.is_open()) continue;
        std::string json((std::istreambuf_iterator<char>(f)),
                          std::istreambuf_iterator<char>());

        Builder::PartDef def;
        def.id          = JsonUint(json, "id");
        if (def.id == 0) continue;   // skip if no valid id

        def.name        = JsonStr(json, "name");
        def.meshPath    = JsonStr(json, "mesh");
        def.description = JsonStr(json, "description");
        def.tier        = (uint8_t)JsonUint(json, "tier", 1);

        // Category
        std::string cat = JsonStr(json, "category", "hull");
        if      (cat == "hull")       def.category = Builder::PartCategory::Hull;
        else if (cat == "weapon")     def.category = Builder::PartCategory::Weapon;
        else if (cat == "engine")     def.category = Builder::PartCategory::Engine;
        else if (cat == "shield")     def.category = Builder::PartCategory::Shield;
        else if (cat == "power")      def.category = Builder::PartCategory::Power;
        else if (cat == "utility")    def.category = Builder::PartCategory::Utility;
        else if (cat == "structural") def.category = Builder::PartCategory::Structural;
        else if (cat == "armor")      def.category = Builder::PartCategory::Armor;

        // Stats
        def.stats.mass          = JsonFloat(json, "mass");
        def.stats.hitpoints     = JsonFloat(json, "hp",  100.0f);
        def.stats.powerDraw     = JsonFloat(json, "power_draw");
        def.stats.powerOutput   = JsonFloat(json, "power_output");
        def.stats.thrust        = JsonFloat(json, "thrust");
        def.stats.shieldRegen   = JsonFloat(json, "shield_regen");
        def.stats.armorRating   = JsonFloat(json, "armor");

        // Snap points — scan for "\"id\":" occurrences inside "snapPoints" block
        size_t spBlock = json.find("\"snapPoints\"");
        if (spBlock != std::string::npos) {
            size_t blockStart = json.find('[', spBlock);
            size_t blockEnd   = json.find(']', blockStart);
            if (blockStart != std::string::npos && blockEnd != std::string::npos) {
                std::string block = json.substr(blockStart, blockEnd - blockStart + 1);
                size_t cursor = 0;
                uint32_t snapCounter = 0;
                while ((cursor = block.find("{", cursor)) != std::string::npos) {
                    size_t snapEnd = block.find('}', cursor);
                    if (snapEnd == std::string::npos) break;
                    std::string snapJson = block.substr(cursor, snapEnd - cursor + 1);

                    Builder::SnapPoint sp;
                    sp.id = JsonUint(snapJson, "id", snapCounter);
                    std::string spType = JsonStr(snapJson, "type", "hull");
                    sp.compatibleTypes.push_back(spType);
                    def.snapPoints.push_back(sp);

                    ++snapCounter;
                    cursor = snapEnd + 1;
                }
            }
        }

        m_impl->partLib.Register(std::move(def));
        ++loaded;
    }
    return loaded;
}

size_t NovaForgeBuilderIntegration::LoadShipTemplates(const std::string& shipsDir) {
    size_t loaded = 0;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(shipsDir, ec)) {
        if (ec) break;
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".json") continue;

        std::ifstream f(entry.path().string());
        if (!f.is_open()) continue;
        std::string json((std::istreambuf_iterator<char>(f)),
                          std::istreambuf_iterator<char>());

        std::string name = JsonStr(json, "name");
        if (name.empty()) name = entry.path().stem().string();

        uint32_t sid = OpenBuildSession(BuildTargetType::Ship, name);
        BuildSession* sess = GetSession(sid);
        if (!sess) continue;

        // Ships save their part count / link count for reference only
        // (full placement data requires a dedicated serialize round-trip)
        ++loaded;
    }
    return loaded;
}

void NovaForgeBuilderIntegration::RegisterPart(Builder::PartDef def) {
    m_impl->partLib.Register(std::move(def));
}

size_t NovaForgeBuilderIntegration::PartCount() const {
    return m_impl->partLib.Count();
}

const Builder::PartDef* NovaForgeBuilderIntegration::GetPartDef(uint32_t id) const {
    return m_impl->partLib.Get(id);
}

std::vector<Builder::PartDef> NovaForgeBuilderIntegration::GetPartsByCategory(
    Builder::PartCategory cat) const {
    return m_impl->partLib.GetByCategory(cat);
}

// ── Session management ────────────────────────────────────────────────────────

uint32_t NovaForgeBuilderIntegration::OpenBuildSession(BuildTargetType type,
                                                        const std::string& name) {
    auto sess = std::make_unique<BuildSession>();
    sess->id   = m_impl->nextSessionId++;
    sess->type = type;
    sess->name = name.empty() ? "Unnamed" : name;

    // Wire BuilderRuntime to the session's Assembly + PartLibrary
    sess->runtime.Init(&sess->assembly, &m_impl->partLib);
    sess->runtime.SetMaxParts(type == BuildTargetType::Station ? 2048 : 512);

    uint32_t id = sess->id;
    m_impl->sessions[id] = std::move(sess);

    if (m_impl->activeSessionId == 0) m_impl->activeSessionId = id;
    return id;
}

uint32_t NovaForgeBuilderIntegration::LoadBuildSession(const std::string& filePath) {
    std::ifstream f(filePath);
    if (!f.is_open()) return 0;
    std::string json((std::istreambuf_iterator<char>(f)),
                      std::istreambuf_iterator<char>());

    std::string name = JsonStr(json, "name", fs::path(filePath).stem().string());
    uint32_t typeVal = JsonUint(json, "type", 0);
    BuildTargetType type = static_cast<BuildTargetType>(
        typeVal < 4 ? typeVal : 0);

    uint32_t sid = OpenBuildSession(type, name);
    BuildSession* sess = GetSession(sid);
    if (sess) {
        if (m_impl->callbacks.onSessionLoaded)
            m_impl->callbacks.onSessionLoaded(sid);
    }
    return sid;
}

bool NovaForgeBuilderIntegration::SaveSession(uint32_t sessionId,
                                               const std::string& filePath) {
    BuildSession* sess = GetSession(sessionId);
    if (!sess) return false;

    std::string json = AssemblyToJson(sess->assembly, sess->name, sess->type);
    std::ofstream f(filePath);
    if (!f.is_open()) return false;
    f << json;
    sess->dirty = false;

    if (m_impl->callbacks.onSessionSaved)
        m_impl->callbacks.onSessionSaved(sessionId);
    return true;
}

void NovaForgeBuilderIntegration::CloseSession(uint32_t sessionId) {
    m_impl->sessions.erase(sessionId);
    if (m_impl->activeSessionId == sessionId) m_impl->activeSessionId = 0;
}

void NovaForgeBuilderIntegration::SetActive(uint32_t sessionId) {
    m_impl->activeSessionId = sessionId;
}

uint32_t     NovaForgeBuilderIntegration::ActiveSessionId() const {
    return m_impl->activeSessionId;
}
BuildSession* NovaForgeBuilderIntegration::GetActiveSession() {
    return GetSession(m_impl->activeSessionId);
}
BuildSession* NovaForgeBuilderIntegration::GetSession(uint32_t sessionId) {
    auto it = m_impl->sessions.find(sessionId);
    return it != m_impl->sessions.end() ? it->second.get() : nullptr;
}

std::vector<uint32_t> NovaForgeBuilderIntegration::AllSessionIds() const {
    std::vector<uint32_t> ids;
    ids.reserve(m_impl->sessions.size());
    for (const auto& [id, _] : m_impl->sessions) ids.push_back(id);
    return ids;
}

// ── Build actions ─────────────────────────────────────────────────────────────

void NovaForgeBuilderIntegration::BeginBuild() {
    if (auto* s = GetActiveSession()) s->runtime.BeginBuild();
}

void NovaForgeBuilderIntegration::EndBuild() {
    if (auto* s = GetActiveSession()) s->runtime.EndBuild();
}

bool NovaForgeBuilderIntegration::IsBuilding() const {
    const BuildSession* s = const_cast<NovaForgeBuilderIntegration*>(this)->GetActiveSession();
    return s ? s->runtime.IsBuilding() : false;
}

uint32_t NovaForgeBuilderIntegration::PlacePart(uint32_t defId,
                                                  const float transform[16]) {
    BuildSession* sess = GetActiveSession();
    if (!sess) return 0;

    // Crafting gate — check player inventory before placing
    if (m_impl->craftingGate && !m_impl->craftingGate(defId, 1)) return 0;

    uint32_t instId = sess->runtime.PlacePart(defId, transform);
    if (instId == 0) return 0;

    // Consume the part from the player's inventory
    if (m_impl->consumePart) m_impl->consumePart(defId, 1);

    // Reinitialise damage system for the updated assembly
    sess->damageSystem.InitFromAssembly(sess->assembly, m_impl->partLib);

    sess->dirty = true;

    if (m_impl->callbacks.onPartPlaced)
        m_impl->callbacks.onPartPlaced(sess->id, instId);

    return instId;
}

bool NovaForgeBuilderIntegration::RemovePart(uint32_t instanceId) {
    BuildSession* sess = GetActiveSession();
    if (!sess) return false;
    bool ok = sess->runtime.RemovePart(instanceId);
    if (ok) {
        sess->damageSystem.InitFromAssembly(sess->assembly, m_impl->partLib);
        sess->dirty = true;
        if (m_impl->callbacks.onPartRemoved)
            m_impl->callbacks.onPartRemoved(sess->id, instanceId);
    }
    return ok;
}

bool NovaForgeBuilderIntegration::WeldParts(uint32_t instA, uint32_t snapA,
                                              uint32_t instB, uint32_t snapB) {
    BuildSession* sess = GetActiveSession();
    if (!sess) return false;

    // Validate snap compatibility using SnapRules
    const Builder::PlacedPart* pA = sess->assembly.GetPart(instA);
    const Builder::PlacedPart* pB = sess->assembly.GetPart(instB);
    if (pA && pB) {
        const Builder::PartDef* defA = m_impl->partLib.Get(pA->defId);
        const Builder::PartDef* defB = m_impl->partLib.Get(pB->defId);
        if (defA && defB) {
            const Builder::SnapPoint* spA = nullptr;
            const Builder::SnapPoint* spB = nullptr;
            for (const auto& sp : defA->snapPoints) if (sp.id == snapA) { spA = &sp; break; }
            for (const auto& sp : defB->snapPoints) if (sp.id == snapB) { spB = &sp; break; }
            if (spA && spB && !m_impl->snapRules.CanSnap(*spA, *spB)) return false;
        }
    }

    bool ok = sess->runtime.WeldParts(instA, snapA, instB, snapB);
    if (ok) {
        sess->dirty = true;
        if (m_impl->callbacks.onWelded)
            m_impl->callbacks.onWelded(sess->id, instA, instB);
    }
    return ok;
}

bool NovaForgeBuilderIntegration::UnweldParts(uint32_t instA, uint32_t snapA) {
    BuildSession* sess = GetActiveSession();
    if (!sess) return false;
    // Find the linked part before unwelding
    auto links = sess->assembly.GetLinks(instA);
    uint32_t instB = 0;
    for (const auto& l : links) if (l.fromSnapId == snapA) { instB = l.toPartId; break; }
    sess->runtime.UnweldParts(instA, snapA);
    sess->dirty = true;
    if (m_impl->callbacks.onUnwelded)
        m_impl->callbacks.onUnwelded(sess->id, instA, instB);
    return true;
}

// ── Interior modules ──────────────────────────────────────────────────────────

size_t NovaForgeBuilderIntegration::AddInteriorSlot(const Builder::ModuleSlot& slot) {
    BuildSession* s = GetActiveSession();
    if (!s) return 0;
    return s->interior.AddSlot(slot);
}

bool NovaForgeBuilderIntegration::PlaceModule(size_t slotIndex,
                                               const Builder::InteriorModule& mod) {
    BuildSession* s = GetActiveSession();
    if (!s) return false;
    bool ok = s->interior.PlaceModule(slotIndex, mod);
    if (ok) s->dirty = true;
    return ok;
}

void NovaForgeBuilderIntegration::RemoveModule(size_t slotIndex) {
    if (auto* s = GetActiveSession()) { s->interior.RemoveModule(slotIndex); s->dirty = true; }
}

const Builder::InteriorModule* NovaForgeBuilderIntegration::GetModule(size_t slotIndex) const {
    auto* s = const_cast<NovaForgeBuilderIntegration*>(this)->GetActiveSession();
    return s ? s->interior.GetModule(slotIndex) : nullptr;
}

size_t NovaForgeBuilderIntegration::InteriorSlotCount() const {
    auto* s = const_cast<NovaForgeBuilderIntegration*>(this)->GetActiveSession();
    return s ? s->interior.SlotCount() : 0;
}
size_t NovaForgeBuilderIntegration::InteriorModuleCount() const {
    auto* s = const_cast<NovaForgeBuilderIntegration*>(this)->GetActiveSession();
    return s ? s->interior.TotalModuleCount() : 0;
}

// ── Snap preview ──────────────────────────────────────────────────────────────

std::vector<Builder::SnapCandidate>
NovaForgeBuilderIntegration::FindSnapCandidates(uint32_t newDefId,
                                                  const Builder::SnapPoint& newSnap) const {
    BuildSession* sess = const_cast<NovaForgeBuilderIntegration*>(this)->GetActiveSession();
    if (!sess) return {};

    const Builder::PartDef* newDef = m_impl->partLib.Get(newDefId);
    if (!newDef) return {};

    std::vector<Builder::SnapCandidate> candidates;
    for (const auto& pp : sess->assembly.GetAllParts()) {
        const Builder::PartDef* def = m_impl->partLib.Get(pp.defId);
        if (!def) continue;
        auto results = m_impl->snapRules.FindCompatibleSnaps(*def, newSnap);
        for (auto& c : results) { c.partId = pp.instanceId; candidates.push_back(c); }
    }
    return candidates;
}

// ── Combat / damage ───────────────────────────────────────────────────────────

Builder::PartHealthState NovaForgeBuilderIntegration::ApplyDamage(
    const Builder::DamageEvent& evt) {
    BuildSession* s = GetActiveSession();
    if (!s) return {};
    auto state = s->damageSystem.ApplyDamage(evt);
    if (state.destroyed && m_impl->callbacks.onPartDestroyed)
        m_impl->callbacks.onPartDestroyed(s->id, state);
    return state;
}

void NovaForgeBuilderIntegration::RepairPart(uint32_t instanceId, float amount) {
    if (auto* s = GetActiveSession()) s->damageSystem.RepairPart(instanceId, amount);
}

void NovaForgeBuilderIntegration::RepairAll() {
    BuildSession* sess = GetActiveSession();
    if (!sess) return;
    for (const auto& pp : sess->assembly.GetAllParts()) {
        const Builder::PartDef* def = m_impl->partLib.Get(pp.defId);
        if (def) sess->damageSystem.RepairPart(pp.instanceId, def->stats.hitpoints);
    }
}

bool NovaForgeBuilderIntegration::IsPartDestroyed(uint32_t instanceId) const {
    auto* s = const_cast<NovaForgeBuilderIntegration*>(this)->GetActiveSession();
    return s ? s->damageSystem.IsDestroyed(instanceId) : false;
}

std::vector<uint32_t> NovaForgeBuilderIntegration::GetDestroyedParts() const {
    auto* s = const_cast<NovaForgeBuilderIntegration*>(this)->GetActiveSession();
    return s ? s->damageSystem.GetDestroyedParts() : std::vector<uint32_t>{};
}

// ── Collision ─────────────────────────────────────────────────────────────────

std::vector<Builder::CollisionShape>
NovaForgeBuilderIntegration::BuildCollision() const {
    auto* s = const_cast<NovaForgeBuilderIntegration*>(this)->GetActiveSession();
    if (!s) return {};
    return m_impl->collisionBuilder.BuildForAssembly(s->assembly, m_impl->partLib);
}

// ── Stats ─────────────────────────────────────────────────────────────────────

BuildSessionStats NovaForgeBuilderIntegration::CalculateStats(uint32_t sessionId) const {
    const auto it = m_impl->sessions.find(sessionId);
    if (it == m_impl->sessions.end()) return {};
    const BuildSession& sess = *it->second;

    // Build mass/hp vectors for Assembly helpers
    std::vector<std::pair<uint32_t,float>> masses, hps;
    for (const auto& pp : sess.assembly.GetAllParts()) {
        const Builder::PartDef* def = m_impl->partLib.Get(pp.defId);
        if (def) {
            masses.push_back({pp.defId, def->stats.mass});
            hps.push_back({pp.defId, def->stats.hitpoints});
        }
    }

    BuildSessionStats s{};
    s.partCount     = (uint32_t)sess.assembly.PartCount();
    s.linkCount     = (uint32_t)sess.assembly.LinkCount();
    s.moduleCount   = (uint32_t)sess.interior.TotalModuleCount();
    s.totalMass     = sess.assembly.CalculateTotalMass(masses);
    s.totalHP       = sess.assembly.CalculateTotalHP(hps);
    s.hasPower      = sess.interior.HasPower();
    s.isValid       = sess.assembly.Validate();
    s.destroyedParts = (uint32_t)sess.damageSystem.GetDestroyedParts().size();

    for (const auto& pp : sess.assembly.GetAllParts()) {
        const Builder::PartDef* def = m_impl->partLib.Get(pp.defId);
        if (!def) continue;
        s.totalPowerDraw   += def->stats.powerDraw;
        s.totalPowerOutput += def->stats.powerOutput;
        s.totalThrust      += def->stats.thrust;
    }

    return s;
}

BuildSessionStats NovaForgeBuilderIntegration::CalculateActiveStats() const {
    return CalculateStats(m_impl->activeSessionId);
}

// ── Tick ──────────────────────────────────────────────────────────────────────

void NovaForgeBuilderIntegration::Tick(float dt) {
    if (auto* s = GetActiveSession()) s->runtime.Tick(dt);
}

// ── Snap compatibility ────────────────────────────────────────────────────────

void NovaForgeBuilderIntegration::RegisterSnapCompatibility(const std::string& a,
                                                             const std::string& b) {
    m_impl->snapRules.RegisterCompatibility(a, b);
}

// ── Crafting gate ─────────────────────────────────────────────────────────────

void NovaForgeBuilderIntegration::SetCraftingGate(CraftingGateFn fn) {
    m_impl->craftingGate = std::move(fn);
}
void NovaForgeBuilderIntegration::SetConsumePart(ConsumePartFn fn) {
    m_impl->consumePart = std::move(fn);
}

// ── Callbacks ─────────────────────────────────────────────────────────────────

void NovaForgeBuilderIntegration::SetCallbacks(BuilderIntegrationCallbacks cb) {
    m_impl->callbacks = std::move(cb);
}

} // namespace Runtime
