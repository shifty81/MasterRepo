#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <functional>

namespace IDE {

// ──────────────────────────────────────────────────────────────
// Token types for syntax highlighting
// ──────────────────────────────────────────────────────────────

enum class TokenType {
    Normal, Keyword, Type, String, Comment, Number, Operator,
    Preprocessor, Identifier, Punctuation
};

struct TextToken {
    uint32_t    startCol = 0;
    uint32_t    endCol   = 0;    // exclusive
    TokenType   type     = TokenType::Normal;
};

// One line of tokenised source text
struct HighlightedLine {
    std::string            text;
    std::vector<TextToken> tokens;
};

// ──────────────────────────────────────────────────────────────
// Cursor & selection
// ──────────────────────────────────────────────────────────────

struct EditorCursor {
    uint32_t line   = 0;
    uint32_t column = 0;
};

struct EditorSelection {
    EditorCursor start;
    EditorCursor end;
    bool IsEmpty() const {
        return start.line == end.line && start.column == end.column;
    }
};

// ──────────────────────────────────────────────────────────────
// Breakpoint
// ──────────────────────────────────────────────────────────────

struct Breakpoint {
    uint32_t    line      = 0;
    std::string filePath;
    bool        enabled   = true;
    std::string condition; // optional conditional expression
};

// ──────────────────────────────────────────────────────────────
// Code completion suggestion
// ──────────────────────────────────────────────────────────────

struct CompletionItem {
    std::string label;
    std::string detail;
    std::string insertText;
    TokenType   kind = TokenType::Identifier;
};

// ──────────────────────────────────────────────────────────────
// CodeEditor
// ──────────────────────────────────────────────────────────────

class CodeEditor {
public:
    // File I/O
    bool Open(const std::string& filePath);
    bool Save(const std::string& filePath = "");
    bool IsModified() const { return m_modified; }
    const std::string& FilePath() const { return m_filePath; }

    // Content access
    size_t LineCount() const { return m_lines.size(); }
    const std::string& GetLine(uint32_t lineIdx) const;
    std::string GetAllText() const;
    void SetText(const std::string& text);

    // Editing
    void InsertChar(char c);
    void InsertText(const std::string& text);
    void DeleteChar();        // delete at cursor
    void DeleteCharBack();    // backspace
    void DeleteLine(uint32_t lineIdx);
    void InsertLine(uint32_t lineIdx, const std::string& text = "");

    // Cursor & selection
    const EditorCursor&    GetCursor()    const { return m_cursor; }
    const EditorSelection& GetSelection() const { return m_selection; }
    void SetCursor(uint32_t line, uint32_t col);
    void MoveCursorRight();
    void MoveCursorLeft();
    void MoveCursorUp();
    void MoveCursorDown();
    void SelectAll();
    void ClearSelection();

    // Clipboard
    std::string Copy()  const;
    void        Cut();
    void        Paste(const std::string& text);

    // Undo/redo
    void Undo();
    void Redo();
    bool CanUndo() const;
    bool CanRedo() const;

    // Search
    std::vector<EditorCursor> Find(const std::string& query,
                                    bool caseSensitive = false) const;
    bool Replace(const std::string& query, const std::string& replacement,
                 bool caseSensitive = false);
    int  ReplaceAll(const std::string& query, const std::string& replacement,
                    bool caseSensitive = false);

    // Syntax highlighting (C++ tokeniser)
    HighlightedLine HighlightLine(uint32_t lineIdx) const;
    std::vector<HighlightedLine> HighlightAll() const;

    // Breakpoints
    void ToggleBreakpoint(uint32_t line);
    void AddBreakpoint(const Breakpoint& bp);
    void RemoveBreakpoint(uint32_t line);
    const std::vector<Breakpoint>& Breakpoints() const { return m_breakpoints; }
    bool HasBreakpoint(uint32_t line) const;

    // AI autocomplete callback  — set externally by the IDE
    using CompletionProvider = std::function<
        std::vector<CompletionItem>(const std::string& prefix, const std::string& context)>;
    void SetCompletionProvider(CompletionProvider fn);
    std::vector<CompletionItem> GetCompletions() const;

    // Language
    void SetLanguage(const std::string& lang) { m_language = lang; }
    const std::string& GetLanguage() const     { return m_language; }

private:
    std::vector<std::string> m_lines = {""};
    std::string              m_filePath;
    std::string              m_language = "cpp";
    EditorCursor             m_cursor;
    EditorSelection          m_selection;
    std::vector<Breakpoint>  m_breakpoints;
    bool                     m_modified = false;
    CompletionProvider       m_completionFn;

    // Undo stack entry
    struct UndoEntry { std::vector<std::string> lines; EditorCursor cursor; };
    std::vector<UndoEntry> m_undoStack;
    std::vector<UndoEntry> m_redoStack;

    void pushUndo();
    void clampCursor();

    // C++ keyword set
    static bool IsCppKeyword(const std::string& word);
    static bool IsCppType(const std::string& word);
};

} // namespace IDE
