#include "VoiceCommand.h"
#include <algorithm>
#include <cctype>

namespace atlas::voice {

static std::string ToLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

void VoiceCommandRegistry::Register(const VoiceCommand& cmd) {
    m_commands.push_back(cmd);
}

const VoiceCommand* VoiceCommandRegistry::Match(const std::string& phrase) const {
    std::string lowerPhrase = ToLower(phrase);
    const VoiceCommand* best = nullptr;
    size_t bestLen = 0;
    for (const auto& cmd : m_commands) {
        std::string lowerCmd = ToLower(cmd.phrase);
        if (lowerPhrase.find(lowerCmd) != std::string::npos && lowerCmd.size() > bestLen) {
            best = &cmd;
            bestLen = lowerCmd.size();
        }
    }
    return best;
}

size_t VoiceCommandRegistry::CommandCount() const {
    return m_commands.size();
}

std::vector<VoiceCommand> VoiceCommandRegistry::GetAll() const {
    return m_commands;
}

std::vector<VoiceCommand> VoiceCommandRegistry::GetByContext(const std::string& context) const {
    std::vector<VoiceCommand> result;
    for (const auto& cmd : m_commands) {
        if (cmd.context == context || cmd.context == "both") {
            result.push_back(cmd);
        }
    }
    return result;
}

} // namespace atlas::voice
