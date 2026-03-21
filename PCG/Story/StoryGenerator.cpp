#include "PCG/Story/StoryGenerator.h"
#include <ctime>
#include <algorithm>
#include <cstring>

namespace PCG {

// ── Deterministic LCG ─────────────────────────────────────────────────────
struct RNG {
    uint64_t s;
    explicit RNG(uint64_t seed) : s(seed ? seed : 1) {}
    uint64_t Next() {
        s = s * 6364136223846793005ULL + 1442695040888963407ULL;
        return s;
    }
    int Pick(int n) { return n > 0 ? static_cast<int>(Next() % static_cast<uint64_t>(n)) : 0; }
    float Float() { return static_cast<float>(Next() >> 11) / static_cast<float>(1ULL << 53); }
};

// ── Beat templates ────────────────────────────────────────────────────────
struct BeatTemplate {
    BeatType    type;
    std::string titleFmt;
    std::string descFmt;
    float       tension;
};

static const BeatTemplate kBeats[] = {
    {BeatType::Intro,          "A New Beginning",        "The journey starts at {location}.",              0.1f},
    {BeatType::IncitingEvent,  "Trouble in {location}",  "An unexpected event disrupts the status quo.",   0.3f},
    {BeatType::RisingAction,   "Rising Tension",          "Conflict with {faction} escalates.",            0.55f},
    {BeatType::RisingAction,   "A Desperate Gambit",     "Risky choices must be made.",                    0.65f},
    {BeatType::Climax,         "The Final Confrontation", "Everything comes to a head.",                   0.9f},
    {BeatType::FallingAction,  "Aftermath",               "The dust settles; wounds licked.",              0.5f},
    {BeatType::Resolution,     "A New Order",             "The world settles into a new normal.",          0.15f},
};

static std::string Fill(const std::string& tmpl, const std::string& location,
                         const std::string& faction)
{
    std::string out = tmpl;
    auto Replace = [&](const std::string& from, const std::string& to) {
        size_t pos = 0;
        while ((pos = out.find(from, pos)) != std::string::npos) {
            out.replace(pos, from.size(), to);
            pos += to.size();
        }
    };
    Replace("{location}", location.empty() ? "the frontier" : location);
    Replace("{faction}", faction.empty() ? "unknown forces" : faction);
    return out;
}

// ── Impl ─────────────────────────────────────────────────────────────────
struct StoryGenerator::Impl {
    std::vector<std::string> themes{
        "survival", "revenge", "discovery", "redemption",
        "conspiracy", "war", "diplomacy"};
    uint32_t nextArcId{1};
    uint32_t nextBeatId{1};
};

StoryGenerator::StoryGenerator() : m_impl(new Impl()) {}
StoryGenerator::~StoryGenerator() { delete m_impl; }

void StoryGenerator::RegisterTheme(const std::string& theme) {
    m_impl->themes.push_back(theme);
}
size_t StoryGenerator::ThemeCount() const { return m_impl->themes.size(); }

StoryArc StoryGenerator::Generate(const WorldContext& ctx) {
    return GenerateWithCallback(ctx, nullptr);
}

StoryArc StoryGenerator::GenerateWithCallback(const WorldContext& ctx,
                                               BeatCallback cb)
{
    uint64_t seed = ctx.seed ? ctx.seed :
        static_cast<uint64_t>(std::time(nullptr));
    RNG rng(seed);

    StoryArc arc;
    arc.id = m_impl->nextArcId++;
    const std::string& theme =
        m_impl->themes[static_cast<size_t>(rng.Pick(static_cast<int>(m_impl->themes.size())))];
    arc.theme = theme;
    arc.title = "Arc of " + theme;

    const std::string& loc =
        ctx.currentLocationID.empty() ? "the frontier" : ctx.currentLocationID;
    const std::string& faction =
        ctx.activeFactionIDs.empty() ? "" : ctx.activeFactionIDs[
            static_cast<size_t>(rng.Pick(static_cast<int>(ctx.activeFactionIDs.size())))];

    // Select active quests to weave in
    std::vector<std::string> questPool = ctx.activeQuestIDs;

    for (const auto& tmpl : kBeats) {
        NarrativeBeat beat;
        beat.id          = m_impl->nextBeatId++;
        beat.type        = tmpl.type;
        beat.title       = Fill(tmpl.titleFmt, loc, faction);
        beat.description = Fill(tmpl.descFmt, loc, faction);
        beat.tension     = tmpl.tension;
        beat.locationID  = loc;
        beat.factionID   = faction;

        // Attach a quest if available
        if (!questPool.empty() && tmpl.type != BeatType::Intro) {
            size_t qi = static_cast<size_t>(rng.Pick(static_cast<int>(questPool.size())));
            beat.questID = questPool[qi];
        }

        arc.beats.push_back(beat);
        if (cb) cb(beat);
    }

    // Optional side quest beat
    if (!questPool.empty() && questPool.size() > 1) {
        NarrativeBeat side;
        side.id          = m_impl->nextBeatId++;
        side.type        = BeatType::SideQuest;
        size_t qi = static_cast<size_t>(rng.Pick(static_cast<int>(questPool.size())));
        side.questID     = questPool[qi];
        side.title       = "A Detour Worth Taking";
        side.description = "An optional path reveals hidden rewards.";
        side.tension     = 0.3f;
        side.locationID  = loc;
        arc.beats.push_back(side);
        if (cb) cb(side);
    }

    arc.completionRatio = 0.0f;
    return arc;
}

bool StoryGenerator::CompleteBeat(StoryArc& arc, uint32_t beatId) {
    size_t completed = 0, total = arc.beats.size();
    for (auto& b : arc.beats) {
        if (b.id == beatId) b.tension = -1.0f; // mark done
        if (b.tension < 0.0f) ++completed;
    }
    arc.completionRatio = total > 0
        ? static_cast<float>(completed) / static_cast<float>(total) : 1.0f;
    return arc.completionRatio >= 1.0f;
}

const NarrativeBeat* StoryGenerator::NextBeat(const StoryArc& arc) const {
    const NarrativeBeat* best = nullptr;
    for (const auto& b : arc.beats) {
        if (b.tension < 0.0f) continue; // already done
        if (!best || b.tension < best->tension) best = &b;
    }
    return best;
}

std::vector<std::string> StoryGenerator::AvailableThemes() const {
    return m_impl->themes;
}

} // namespace PCG
