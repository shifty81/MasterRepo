#pragma once
// ============================================================
// Atlas AI Asset Decision Framework
// ============================================================
//
// Provides intelligent, context-aware asset suggestions for
// game projects. The framework analyzes:
//
// - Current project state (via ProjectContext)
// - Missing assets and requirements
// - Available asset sources (procedural, marketplace)
// - Game type-specific needs
//
// The framework integrates with AtlasAICore for processing
// requests and with MarketplaceImporter for external assets.
//
// See: docs/16_ATLAS_AI.md

#include "ProjectContext.h"
#include "AtlasAICore.h"
#include "../assets/MarketplaceImporter.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace atlas::ai {

/// Result of an asset decision process
struct AssetDecision {
    bool shouldGenerate = false;     // Generate procedurally
    bool shouldFetch = false;        // Fetch from marketplace
    AssetSuggestion suggestion;      // The underlying suggestion
    std::string marketplace;         // Which marketplace to use
    std::string assetId;             // External asset ID if fetching
    double confidence = 0.0;         // AI confidence in this decision
    std::string reasoning;           // Human-readable reasoning
};

/// Action the framework can take on a decision
enum class DecisionAction : uint8_t {
    None,           // No action
    Generate,       // Generate procedurally
    Fetch,          // Fetch from external source
    Suggest,        // Just suggest, don't act
    Skip            // Skip this suggestion
};

/// Callback for decision approval workflow
using DecisionApprovalCallback = std::function<DecisionAction(const AssetDecision&)>;

/// Configuration for the decision framework
struct AssetDecisionConfig {
    bool autoApprove = false;         // Auto-approve decisions above threshold
    double autoApproveThreshold = 0.9; // Confidence threshold for auto-approval
    uint32_t maxSuggestionsPerRun = 5; // Max suggestions per analysis
    bool preferProcedural = true;     // Prefer procedural generation when possible
    std::string cacheDirectory;       // Directory for caching fetched assets
};

/// Main AI Asset Decision Framework
class AIAssetDecisionFramework {
public:
    AIAssetDecisionFramework();
    ~AIAssetDecisionFramework();

    // --- Configuration ---
    void SetConfig(const AssetDecisionConfig& config);
    const AssetDecisionConfig& GetConfig() const;

    // --- Context Management ---
    void SetProjectContext(std::shared_ptr<ProjectContext> context);
    std::shared_ptr<ProjectContext> GetProjectContext() const;
    
    // --- Marketplace Integration ---
    void SetMarketplaceRegistry(asset::MarketplaceImportRegistry* registry);

    // --- AI Core Integration ---
    void RegisterWithAICore(AtlasAICore& core);
    void UnregisterFromAICore(AtlasAICore& core);

    // --- Decision Making ---
    
    /// Analyze the project and generate asset decisions
    std::vector<AssetDecision> AnalyzeProject();
    
    /// Get decisions for a specific asset category
    std::vector<AssetDecision> AnalyzeCategory(AssetCategory category);
    
    /// Execute a decision (generate or fetch)
    bool ExecuteDecision(const AssetDecision& decision);
    
    /// Execute all approved decisions
    uint32_t ExecuteApprovedDecisions(const std::vector<AssetDecision>& decisions,
                                       DecisionApprovalCallback approvalCallback);

    // --- Suggestion Queries ---
    
    /// Get top priority missing assets
    std::vector<AssetSuggestion> GetTopMissingAssets(uint32_t count) const;
    
    /// Check if a specific asset type is needed
    bool IsAssetNeeded(AssetCategory category) const;
    
    /// Get recommended sources for an asset category
    std::vector<std::string> GetRecommendedSources(AssetCategory category) const;

    // --- Intent Handlers ---
    AIResponse HandleAnalyzeIntent(const AIRequest& request);
    AIResponse HandleSuggestIntent(const AIRequest& request);
    AIResponse HandleFetchIntent(const AIRequest& request);

    // --- Statistics ---
    uint32_t GetDecisionCount() const;
    uint32_t GetExecutedCount() const;
    void ClearStatistics();

private:
    AssetDecision CreateDecision(const AssetSuggestion& suggestion) const;
    std::string DetermineMarketplace(AssetCategory category) const;
    bool CanGenerateProcedurally(AssetCategory category) const;
    double CalculateConfidence(const AssetSuggestion& suggestion) const;

    AssetDecisionConfig m_config;
    std::shared_ptr<ProjectContext> m_context;
    asset::MarketplaceImportRegistry* m_marketplaceRegistry = nullptr;
    
    // Statistics
    uint32_t m_decisionCount = 0;
    uint32_t m_executedCount = 0;
};

} // namespace atlas::ai
