#include "AI/Tutorial/Tutorial.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace AI {

// ── Singleton ──────────────────────────────────────────────────

TutorialSystem& TutorialSystem::Get() {
    static TutorialSystem instance;
    return instance;
}

// ── Library management ─────────────────────────────────────────

void TutorialSystem::RegisterTutorial(const Tutorial& t) {
    m_tutorials[t.id] = t;
}

void TutorialSystem::RemoveTutorial(const std::string& id) {
    m_tutorials.erase(id);
}

void TutorialSystem::ClearLibrary() {
    m_tutorials.clear();
    m_activeTutorialId.clear();
}

void TutorialSystem::LoadFromDirectory(const std::string& dir) {
    if (!fs::exists(dir)) return;
    // Each sub-directory is a tutorial; each .txt/.md file is a step
    for (auto& entry : fs::directory_iterator(dir)) {
        if (!entry.is_directory()) continue;
        Tutorial t;
        t.id    = entry.path().filename().string();
        t.title = t.id;

        std::vector<fs::path> stepFiles;
        for (auto& sf : fs::directory_iterator(entry.path())) {
            if (sf.path().extension() == ".md" ||
                sf.path().extension() == ".txt") {
                stepFiles.push_back(sf.path());
            }
        }
        std::sort(stepFiles.begin(), stepFiles.end());

        for (auto& sf : stepFiles) {
            TutorialStep step;
            step.id    = sf.filename().string();
            step.title = sf.stem().string();
            std::ifstream f(sf);
            std::ostringstream oss;
            oss << f.rdbuf();
            step.instructions = oss.str();
            t.steps.push_back(std::move(step));
        }
        if (!t.steps.empty()) RegisterTutorial(t);
    }
}

// ── AI generation ──────────────────────────────────────────────

Tutorial TutorialSystem::GenerateTutorial(const std::string& featureName,
                                           const std::string& targetFile) {
    Tutorial t;
    t.id       = "generated_" + featureName;
    t.title    = "How to use " + featureName;
    t.category = "generated";
    t.description = "Auto-generated tutorial for " + featureName;

    // Step 1: overview
    {
        TutorialStep s;
        s.id           = "intro";
        s.title        = "Introduction";
        s.instructions = "Welcome to the " + featureName + " tutorial.\n"
                         "This walkthrough will guide you through the main features.";
        t.steps.push_back(s);
    }

    // If a source file is provided, generate a step per public method (heuristic)
    if (!targetFile.empty()) {
        std::ifstream f(targetFile);
        if (f.is_open()) {
            std::string line;
            int idx = 0;
            while (std::getline(f, line)) {
                if (line.find("void ") != std::string::npos ||
                    line.find("bool ") != std::string::npos) {
                    TutorialStep s;
                    s.id           = "step_" + std::to_string(++idx);
                    s.title        = "Step " + std::to_string(idx);
                    s.instructions = "Explore: `" + line + "`";
                    s.codeSnippet  = line;
                    t.steps.push_back(s);
                    if (idx >= 5) break; // limit auto-steps
                }
            }
        }
    }

    // Final step
    {
        TutorialStep s;
        s.id           = "done";
        s.title        = "Completed";
        s.instructions = "Congratulations! You have completed the " + featureName + " tutorial.";
        t.steps.push_back(s);
    }

    RegisterTutorial(t);
    return t;
}

// ── Playback ───────────────────────────────────────────────────

bool TutorialSystem::StartTutorial(const std::string& id) {
    auto it = m_tutorials.find(id);
    if (it == m_tutorials.end()) return false;
    m_activeTutorialId = id;
    it->second.m_currentStep = 0;
    it->second.completed      = false;
    if (m_onStepChanged && !it->second.steps.empty()) {
        m_onStepChanged(it->second.steps[0]);
    }
    return true;
}

void TutorialSystem::NextStep() {
    auto it = m_tutorials.find(m_activeTutorialId);
    if (it == m_tutorials.end()) return;
    Tutorial& t = it->second;
    if (t.m_currentStep + 1 < t.steps.size()) {
        ++t.m_currentStep;
        if (m_onStepChanged) m_onStepChanged(t.steps[t.m_currentStep]);
    } else {
        FinishTutorial();
    }
}

void TutorialSystem::PreviousStep() {
    auto it = m_tutorials.find(m_activeTutorialId);
    if (it == m_tutorials.end()) return;
    Tutorial& t = it->second;
    if (t.m_currentStep > 0) {
        --t.m_currentStep;
        if (m_onStepChanged) m_onStepChanged(t.steps[t.m_currentStep]);
    }
}

void TutorialSystem::SkipStep() { NextStep(); }

void TutorialSystem::FinishTutorial() {
    auto it = m_tutorials.find(m_activeTutorialId);
    if (it == m_tutorials.end()) return;
    it->second.completed = true;
    if (m_onCompleted) m_onCompleted(it->second);
    m_activeTutorialId.clear();
}

void TutorialSystem::AbortTutorial() {
    m_activeTutorialId.clear();
}

bool TutorialSystem::ValidateCurrentStep(const std::string& userAction) {
    const TutorialStep* step = CurrentStep();
    if (!step) return false;
    if (step->expectedAction.empty()) return true;
    return userAction.find(step->expectedAction) != std::string::npos;
}

// ── Query ──────────────────────────────────────────────────────

const Tutorial* TutorialSystem::ActiveTutorial() const {
    auto it = m_tutorials.find(m_activeTutorialId);
    return (it != m_tutorials.end()) ? &it->second : nullptr;
}

const TutorialStep* TutorialSystem::CurrentStep() const {
    const Tutorial* t = ActiveTutorial();
    if (!t || t->steps.empty()) return nullptr;
    return &t->steps[t->m_currentStep];
}

std::vector<const Tutorial*> TutorialSystem::ListByCategory(const std::string& category) const {
    std::vector<const Tutorial*> out;
    for (auto& [id, t] : m_tutorials) {
        if (t.category == category) out.push_back(&t);
    }
    return out;
}

std::vector<const Tutorial*> TutorialSystem::ListAll() const {
    std::vector<const Tutorial*> out;
    for (auto& [id, t] : m_tutorials) out.push_back(&t);
    return out;
}

size_t TutorialSystem::CompletedCount() const {
    return static_cast<size_t>(
        std::count_if(m_tutorials.begin(), m_tutorials.end(),
                      [](const auto& kv){ return kv.second.completed; }));
}

// ── Callbacks ──────────────────────────────────────────────────

void TutorialSystem::OnStepChanged(StepCallback cb)          { m_onStepChanged = std::move(cb); }
void TutorialSystem::OnTutorialCompleted(CompletedCallback cb){ m_onCompleted   = std::move(cb); }

} // namespace AI
