#pragma once
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

namespace AI {

// ──────────────────────────────────────────────────────────────
// Tutorial step
// ──────────────────────────────────────────────────────────────

struct TutorialStep {
    std::string id;
    std::string title;
    std::string instructions;       // Markdown / plain text
    std::string highlightElement;   // UI element ID to highlight
    std::string expectedAction;     // action the user must perform to advance
    bool        optional = false;
    std::string codeSnippet;        // optional code sample to display
};

// ──────────────────────────────────────────────────────────────
// Tutorial — a named walkthrough consisting of ordered steps
// ──────────────────────────────────────────────────────────────

struct Tutorial {
    std::string                id;
    std::string                title;
    std::string                description;
    std::string                category;    // e.g. "editor", "scripting", "pcg"
    std::vector<TutorialStep>  steps;
    bool                       completed = false;
    size_t CurrentStepIndex()  const { return m_currentStep; }
private:
    size_t m_currentStep = 0;
    friend class TutorialSystem;
};

// ──────────────────────────────────────────────────────────────
// TutorialSystem — AI-driven tutorial manager
// ──────────────────────────────────────────────────────────────

class TutorialSystem {
public:
    // Library management
    void RegisterTutorial(const Tutorial& t);
    void RemoveTutorial(const std::string& id);
    void LoadFromDirectory(const std::string& dir);
    void ClearLibrary();

    // AI generation: auto-create a tutorial from a feature description
    Tutorial GenerateTutorial(const std::string& featureName,
                               const std::string& targetFile = "");

    // Playback
    bool StartTutorial(const std::string& id);
    void NextStep();
    void PreviousStep();
    void SkipStep();
    void FinishTutorial();
    void AbortTutorial();

    // Validation: returns true if the user has satisfied the current step
    bool ValidateCurrentStep(const std::string& userAction);

    // Query
    const Tutorial*              ActiveTutorial() const;
    const TutorialStep*          CurrentStep()   const;
    std::vector<const Tutorial*> ListByCategory(const std::string& category) const;
    std::vector<const Tutorial*> ListAll()       const;
    size_t                       CompletedCount() const;

    // Callbacks
    using StepCallback     = std::function<void(const TutorialStep&)>;
    using CompletedCallback = std::function<void(const Tutorial&)>;
    void OnStepChanged(StepCallback cb);
    void OnTutorialCompleted(CompletedCallback cb);

    // Singleton
    static TutorialSystem& Get();

private:
    std::unordered_map<std::string, Tutorial> m_tutorials;
    std::string                               m_activeTutorialId;
    StepCallback                              m_onStepChanged;
    CompletedCallback                         m_onCompleted;
};

} // namespace AI
