#include "GUIInputRecorder.h"
#include <fstream>

namespace atlas::ui {

static constexpr uint32_t kMagic = 0x41475549;  // "AGUI" in little-endian

void GUIInputRecorder::StartRecording(uint64_t startTick) {
    m_log = GUIInputLog{};
    m_log.startTick = startTick;
    m_state = GUIRecordState::Recording;
}

void GUIInputRecorder::StopRecording() {
    if (m_state != GUIRecordState::Recording) return;
    m_log.endTick = m_log.events.empty() ? m_log.startTick : m_log.events.back().tick;
    m_state = GUIRecordState::Idle;
}

void GUIInputRecorder::RecordEvent(const UICommand& cmd) {
    if (m_state != GUIRecordState::Recording) return;
    GUIInputEvent evt;
    evt.type = cmd.type;
    evt.targetWidgetId = cmd.targetWidgetId;
    evt.tick = cmd.tick;
    evt.valueFloat = cmd.valueFloat;
    evt.valueString = cmd.valueString;
    m_log.events.push_back(std::move(evt));
}

void GUIInputRecorder::StartPlayback(UICommandBus* bus, uint64_t startTick) {
    m_playbackBus = bus;
    m_playbackStartTick = startTick;
    m_playbackPos = 0;
    m_state = GUIRecordState::Playing;
}

void GUIInputRecorder::StopPlayback() {
    m_state = GUIRecordState::Idle;
    m_playbackBus = nullptr;
    m_playbackPos = 0;
}

void GUIInputRecorder::AdvancePlayback(uint64_t currentTick) {
    if (m_state != GUIRecordState::Playing || m_playbackBus == nullptr) return;

    while (m_playbackPos < m_log.events.size()) {
        const auto& evt = m_log.events[m_playbackPos];
        // Clamp to zero if event tick precedes log start (malformed data);
        // events fire immediately at playback start rather than being dropped.
        uint64_t offset = (evt.tick >= m_log.startTick) ? (evt.tick - m_log.startTick) : 0;
        uint64_t rebasedTick = offset + m_playbackStartTick;
        if (rebasedTick > currentTick) break;

        UICommand cmd;
        cmd.type = evt.type;
        cmd.targetWidgetId = evt.targetWidgetId;
        cmd.tick = currentTick;
        cmd.valueFloat = evt.valueFloat;
        cmd.valueString = evt.valueString;
        m_playbackBus->Enqueue(std::move(cmd));
        ++m_playbackPos;
    }
}

bool GUIInputRecorder::SaveLog(const std::string& path) const {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) return false;

    file.write(reinterpret_cast<const char*>(&kMagic), sizeof(kMagic));
    file.write(reinterpret_cast<const char*>(&m_log.version), sizeof(m_log.version));
    file.write(reinterpret_cast<const char*>(&m_log.startTick), sizeof(m_log.startTick));
    file.write(reinterpret_cast<const char*>(&m_log.endTick), sizeof(m_log.endTick));

    uint32_t eventCount = static_cast<uint32_t>(m_log.events.size());
    file.write(reinterpret_cast<const char*>(&eventCount), sizeof(eventCount));

    for (const auto& evt : m_log.events) {
        auto rawType = static_cast<uint8_t>(evt.type);
        file.write(reinterpret_cast<const char*>(&rawType), sizeof(rawType));
        file.write(reinterpret_cast<const char*>(&evt.targetWidgetId), sizeof(evt.targetWidgetId));
        file.write(reinterpret_cast<const char*>(&evt.tick), sizeof(evt.tick));
        file.write(reinterpret_cast<const char*>(&evt.valueFloat), sizeof(evt.valueFloat));

        uint32_t strLen = static_cast<uint32_t>(evt.valueString.size());
        file.write(reinterpret_cast<const char*>(&strLen), sizeof(strLen));
        if (strLen > 0) {
            file.write(evt.valueString.data(), static_cast<std::streamsize>(strLen));
        }
    }

    return file.good();
}

bool GUIInputRecorder::LoadLog(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return false;

    uint32_t magic = 0;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    if (magic != kMagic) return false;

    GUIInputLog log;
    file.read(reinterpret_cast<char*>(&log.version), sizeof(log.version));
    if (log.version != 1) return false;

    file.read(reinterpret_cast<char*>(&log.startTick), sizeof(log.startTick));
    file.read(reinterpret_cast<char*>(&log.endTick), sizeof(log.endTick));

    uint32_t eventCount = 0;
    file.read(reinterpret_cast<char*>(&eventCount), sizeof(eventCount));

    static constexpr uint32_t kMaxEvents = 16 * 1024 * 1024;
    if (eventCount > kMaxEvents) return false;

    log.events.reserve(eventCount);
    for (uint32_t i = 0; i < eventCount; ++i) {
        GUIInputEvent evt;
        uint8_t rawType = 0;
        file.read(reinterpret_cast<char*>(&rawType), sizeof(rawType));
        evt.type = static_cast<UICommandType>(rawType);

        file.read(reinterpret_cast<char*>(&evt.targetWidgetId), sizeof(evt.targetWidgetId));
        file.read(reinterpret_cast<char*>(&evt.tick), sizeof(evt.tick));
        file.read(reinterpret_cast<char*>(&evt.valueFloat), sizeof(evt.valueFloat));

        uint32_t strLen = 0;
        file.read(reinterpret_cast<char*>(&strLen), sizeof(strLen));

        static constexpr uint32_t kMaxStringLen = 64 * 1024;
        if (strLen > kMaxStringLen) return false;

        if (strLen > 0) {
            evt.valueString.resize(strLen);
            file.read(evt.valueString.data(), static_cast<std::streamsize>(strLen));
        }

        log.events.push_back(std::move(evt));
    }

    if (file.fail() && !file.eof()) return false;

    m_log = std::move(log);
    m_state = GUIRecordState::Idle;
    return true;
}

GUIRecordState GUIInputRecorder::State() const {
    return m_state;
}

const GUIInputLog& GUIInputRecorder::Log() const {
    return m_log;
}

size_t GUIInputRecorder::EventCount() const {
    return m_log.events.size();
}

size_t GUIInputRecorder::PlaybackPosition() const {
    return m_playbackPos;
}

} // namespace atlas::ui
