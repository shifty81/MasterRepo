#pragma once
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

namespace atlas::voice {

struct VoiceCommand {
    std::string phrase;
    std::string intentName;
    std::string context; // "game", "editor", "both"
};

class VoiceCommandRegistry {
public:
    void Register(const VoiceCommand& cmd);
    const VoiceCommand* Match(const std::string& phrase) const;
    size_t CommandCount() const;
    std::vector<VoiceCommand> GetAll() const;
    std::vector<VoiceCommand> GetByContext(const std::string& context) const;
private:
    std::vector<VoiceCommand> m_commands;
};

} // namespace atlas::voice
