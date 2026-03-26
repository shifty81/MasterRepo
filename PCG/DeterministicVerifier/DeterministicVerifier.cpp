#include "PCG/DeterministicVerifier/DeterministicVerifier.h"
#include <chrono>
#include <functional>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace PCG {

static uint64_t NowMs() {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
}

struct DeterministicVerifier::Impl {
    std::unordered_map<std::string,
        std::function<std::string(uint64_t)>>   generators;
    std::vector<VerifyResult>                   history;
    std::function<void(const VerifyResult&)>    onFailed;
    std::function<void(const VerifyResult&)>    onPassed;
};

DeterministicVerifier::DeterministicVerifier() : m_impl(new Impl()) {}
DeterministicVerifier::~DeterministicVerifier() { delete m_impl; }

void DeterministicVerifier::Init()     {}
void DeterministicVerifier::Shutdown() {}

void DeterministicVerifier::RegisterGenerator(
    const std::string& name,
    std::function<std::string(uint64_t)> generator) {
    m_impl->generators[name] = std::move(generator);
}

void DeterministicVerifier::UnregisterGenerator(const std::string& name) {
    m_impl->generators.erase(name);
}

VerifyResult DeterministicVerifier::Verify(const std::string& name,
                                           uint64_t seed) {
    VerifyResult result;
    result.generatorName = name;
    result.seed          = seed;

    auto it = m_impl->generators.find(name);
    if (it == m_impl->generators.end()) {
        result.passed  = false;
        result.details = "Generator '" + name + "' not registered.";
        return result;
    }

    uint64_t t0 = NowMs();
    std::string out1 = it->second(seed);
    std::string out2 = it->second(seed);
    result.runDurationMs = NowMs() - t0;

    result.passed = (out1 == out2);
    if (!result.passed) {
        std::ostringstream ss;
        ss << "Non-determinism detected. Run1 length=" << out1.size()
           << " Run2 length=" << out2.size();
        if (out1.size() < 256 && out2.size() < 256)
            ss << "\nRun1: " << out1 << "\nRun2: " << out2;
        result.details = ss.str();
        if (m_impl->onFailed) m_impl->onFailed(result);
    } else {
        result.details = "Outputs match (" + std::to_string(out1.size()) + " bytes).";
        if (m_impl->onPassed) m_impl->onPassed(result);
    }

    m_impl->history.push_back(result);
    return result;
}

std::vector<VerifyResult>
DeterministicVerifier::VerifyAll(const std::vector<uint64_t>& seeds) {
    std::vector<VerifyResult> results;
    for (auto& [name, _] : m_impl->generators)
        for (uint64_t seed : seeds)
            results.push_back(Verify(name, seed));
    return results;
}

uint64_t DeterministicVerifier::MakeSeed(const std::string& sessionKey) {
    // FNV-1a hash for a reproducible seed from a string
    uint64_t hash = 14695981039346656037ULL;
    for (unsigned char c : sessionKey) {
        hash ^= static_cast<uint64_t>(c);
        hash *= 1099511628211ULL;
    }
    return hash;
}

void DeterministicVerifier::OnVerifyFailed(
    std::function<void(const VerifyResult&)> cb) {
    m_impl->onFailed = std::move(cb);
}

void DeterministicVerifier::OnVerifyPassed(
    std::function<void(const VerifyResult&)> cb) {
    m_impl->onPassed = std::move(cb);
}

std::vector<VerifyResult> DeterministicVerifier::History() const {
    return m_impl->history;
}

void DeterministicVerifier::ClearHistory() {
    m_impl->history.clear();
}

} // namespace PCG
