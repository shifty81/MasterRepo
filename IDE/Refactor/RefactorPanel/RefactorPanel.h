#pragma once
/**
 * @file RefactorPanel.h
 * @brief IDE refactoring panel: rename symbol, extract function/variable, move declaration.
 *
 * RefactorPanel provides high-level code refactoring operations over a set of
 * indexed source files without requiring a full compiler:
 *
 *   RefactorKind: Rename, ExtractFunction, ExtractVariable, InlineVariable,
 *                 MoveDeclaration, ChangeSignature.
 *
 *   RefactorLocation: filePath, line (1-based), column (0-based).
 *
 *   RefactorRequest:
 *     - kind, target location (symbol position), new name / extracted name,
 *       scope (function body start/end lines for extraction),
 *       targetFile (for move), parameterList (for signature change).
 *
 *   TextEdit: filePath, startLine, startCol, endLine, endCol, newText.
 *   RefactorResult: success, edits (apply in reverse order), diagnostics.
 *
 *   RefactorPanel:
 *     - IndexFiles(paths): build a symbol/scope index for refactoring.
 *     - IndexFile(path): (re-)index a single file.
 *     - Preview(request): compute edits without applying them.
 *     - Apply(request): compute and write edits to disk.
 *     - Undo(): reverse the last applied refactoring (single-level undo).
 *     - GetAvailableRefactors(loc): list applicable RefactorKinds at location.
 *     - OnApplyComplete(cb) / OnIndexReady(cb) callbacks.
 *     - RefactorStats: indexedFiles, totalRefactors, failedRefactors.
 */

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace IDE {

// ── Refactor kinds ────────────────────────────────────────────────────────
enum class RefactorKind : uint8_t {
    Rename,
    ExtractFunction,
    ExtractVariable,
    InlineVariable,
    MoveDeclaration,
    ChangeSignature
};

// ── Source location ───────────────────────────────────────────────────────
struct RefactorLocation {
    std::string filePath;
    uint32_t    line{1};    ///< 1-based
    uint32_t    column{0};  ///< 0-based
};

// ── Refactoring request ───────────────────────────────────────────────────
struct RefactorRequest {
    RefactorKind     kind{RefactorKind::Rename};
    RefactorLocation location;          ///< Caret / symbol position
    std::string      newName;           ///< Rename / extract: new identifier
    uint32_t         scopeStartLine{0}; ///< Extract: start of selection
    uint32_t         scopeEndLine{0};   ///< Extract: end of selection
    std::string      targetFile;        ///< Move: destination file path
    std::string      parameterList;     ///< ChangeSignature: new param list string
};

// ── Text edit ─────────────────────────────────────────────────────────────
struct TextEdit {
    std::string filePath;
    uint32_t    startLine{1};
    uint32_t    startCol{0};
    uint32_t    endLine{1};
    uint32_t    endCol{0};
    std::string newText;
};

// ── Refactor result ───────────────────────────────────────────────────────
struct RefactorResult {
    bool                     success{false};
    std::vector<TextEdit>    edits;        ///< Apply in reverse order for safety
    std::vector<std::string> diagnostics;  ///< Warnings / errors
};

// ── Stats ─────────────────────────────────────────────────────────────────
struct RefactorStats {
    uint32_t indexedFiles{0};
    uint32_t totalRefactors{0};
    uint32_t failedRefactors{0};
    double   avgRefactorMs{0.0};
};

// ── Callbacks ─────────────────────────────────────────────────────────────
using ApplyCompleteCb  = std::function<void(const RefactorResult&)>;
using IndexReadyCb     = std::function<void(uint32_t fileCount)>;

// ── RefactorPanel ─────────────────────────────────────────────────────────
class RefactorPanel {
public:
    RefactorPanel();
    ~RefactorPanel();

    RefactorPanel(const RefactorPanel&) = delete;
    RefactorPanel& operator=(const RefactorPanel&) = delete;

    // ── indexing ──────────────────────────────────────────────
    void IndexFiles(const std::vector<std::string>& paths);
    void IndexFile (const std::string& path);
    uint32_t GetIndexedFileCount() const;

    // ── refactoring ───────────────────────────────────────────
    RefactorResult Preview(const RefactorRequest& request) const;
    RefactorResult Apply  (const RefactorRequest& request);
    bool           Undo();

    // ── discovery ─────────────────────────────────────────────
    std::vector<RefactorKind> GetAvailableRefactors(const RefactorLocation& loc) const;

    // ── callbacks ─────────────────────────────────────────────
    void OnApplyComplete(ApplyCompleteCb cb);
    void OnIndexReady   (IndexReadyCb    cb);

    // ── stats ─────────────────────────────────────────────────
    RefactorStats GetStats() const;
    void          ResetStats();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace IDE
