#pragma once
#include <string>
#include <vector>

namespace AI {
namespace KnowledgeIngestion {

class IngestionManager {
public:
    // Ingest a PDF or code file and add to the knowledge base
    bool IngestFile(const std::string& filePath);
    // List all ingested documents
    std::vector<std::string> ListDocuments() const;
    // Search the knowledge base
    std::vector<std::string> SemanticSearch(const std::string& query) const;
};

} // namespace KnowledgeIngestion
} // namespace AI
