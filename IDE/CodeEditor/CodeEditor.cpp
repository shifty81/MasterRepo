#include "IDE/CodeEditor/CodeEditor.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <unordered_set>

namespace IDE {

// ──────────────────────────────────────────────────────────────
// Keyword tables
// ──────────────────────────────────────────────────────────────

static const std::unordered_set<std::string> s_cppKeywords = {
    "alignas","alignof","and","and_eq","asm","auto","bitand","bitor","break",
    "case","catch","class","compl","concept","const","consteval","constexpr",
    "constinit","const_cast","continue","co_await","co_return","co_yield",
    "decltype","default","delete","do","dynamic_cast","else","enum","explicit",
    "export","extern","false","for","friend","goto","if","inline","mutable",
    "namespace","new","noexcept","not","not_eq","nullptr","operator","or",
    "or_eq","private","protected","public","register","reinterpret_cast",
    "requires","return","sizeof","static","static_assert","static_cast",
    "struct","switch","template","this","throw","true","try","typedef",
    "typeid","typename","union","using","virtual","volatile","while","xor",
    "xor_eq","override","final","import","module"
};

static const std::unordered_set<std::string> s_cppTypes = {
    "bool","char","char8_t","char16_t","char32_t","double","float","int",
    "long","short","signed","unsigned","void","wchar_t","int8_t","int16_t",
    "int32_t","int64_t","uint8_t","uint16_t","uint32_t","uint64_t","size_t",
    "ptrdiff_t","nullptr_t","string","vector","map","unordered_map","set",
    "unordered_set","pair","tuple","optional","variant","any","span"
};

bool CodeEditor::IsCppKeyword(const std::string& w) { return s_cppKeywords.count(w) > 0; }
bool CodeEditor::IsCppType(const std::string& w)    { return s_cppTypes.count(w)    > 0; }

// ──────────────────────────────────────────────────────────────
// File I/O
// ──────────────────────────────────────────────────────────────

bool CodeEditor::Open(const std::string& filePath) {
    std::ifstream f(filePath);
    if (!f) return false;
    m_lines.clear();
    std::string line;
    while (std::getline(f, line))
        m_lines.push_back(std::move(line));
    if (m_lines.empty()) m_lines.push_back("");
    m_filePath = filePath;
    m_modified = false;
    m_cursor   = {};
    ClearSelection();
    m_undoStack.clear(); m_redoStack.clear();
    // Infer language from extension
    size_t dot = filePath.rfind('.');
    if (dot != std::string::npos) {
        std::string ext = filePath.substr(dot+1);
        if (ext=="h"||ext=="hpp"||ext=="cpp"||ext=="cc"||ext=="cxx") m_language="cpp";
        else if (ext=="py")  m_language="python";
        else if (ext=="lua") m_language="lua";
        else                 m_language="text";
    }
    return true;
}

bool CodeEditor::Save(const std::string& filePath) {
    const std::string& path = filePath.empty() ? m_filePath : filePath;
    if (path.empty()) return false;
    std::ofstream f(path);
    if (!f) return false;
    for (size_t i = 0; i < m_lines.size(); ++i) {
        if (i > 0) f << '\n';
        f << m_lines[i];
    }
    m_filePath = path;
    m_modified = false;
    return true;
}

// ──────────────────────────────────────────────────────────────
// Content access
// ──────────────────────────────────────────────────────────────

const std::string& CodeEditor::GetLine(uint32_t idx) const {
    static const std::string empty;
    return idx < m_lines.size() ? m_lines[idx] : empty;
}

std::string CodeEditor::GetAllText() const {
    std::string out;
    for (size_t i = 0; i < m_lines.size(); ++i) {
        if (i > 0) out += '\n';
        out += m_lines[i];
    }
    return out;
}

void CodeEditor::SetText(const std::string& text) {
    pushUndo();
    m_lines.clear();
    std::istringstream ss(text);
    std::string line;
    while (std::getline(ss, line))
        m_lines.push_back(std::move(line));
    if (m_lines.empty()) m_lines.push_back("");
    m_cursor = {}; ClearSelection();
    m_modified = true;
}

// ──────────────────────────────────────────────────────────────
// Cursor
// ──────────────────────────────────────────────────────────────

void CodeEditor::clampCursor() {
    m_cursor.line = std::min(m_cursor.line, (uint32_t)(m_lines.size() - 1));
    uint32_t len = (uint32_t)m_lines[m_cursor.line].size();
    m_cursor.column = std::min(m_cursor.column, len);
}

void CodeEditor::SetCursor(uint32_t line, uint32_t col) {
    m_cursor = {line, col}; clampCursor();
}

void CodeEditor::MoveCursorRight() {
    const auto& ln = m_lines[m_cursor.line];
    if (m_cursor.column < (uint32_t)ln.size()) { ++m_cursor.column; }
    else if (m_cursor.line + 1 < (uint32_t)m_lines.size()) {
        ++m_cursor.line; m_cursor.column = 0;
    }
}

void CodeEditor::MoveCursorLeft() {
    if (m_cursor.column > 0) { --m_cursor.column; }
    else if (m_cursor.line > 0) {
        --m_cursor.line;
        m_cursor.column = (uint32_t)m_lines[m_cursor.line].size();
    }
}

void CodeEditor::MoveCursorUp() {
    if (m_cursor.line > 0) { --m_cursor.line; clampCursor(); }
}

void CodeEditor::MoveCursorDown() {
    if (m_cursor.line + 1 < (uint32_t)m_lines.size()) { ++m_cursor.line; clampCursor(); }
}

void CodeEditor::SelectAll() {
    m_selection.start = {0, 0};
    m_selection.end   = {(uint32_t)(m_lines.size()-1),
                          (uint32_t)m_lines.back().size()};
}

void CodeEditor::ClearSelection() {
    m_selection.start = m_cursor;
    m_selection.end   = m_cursor;
}

// ──────────────────────────────────────────────────────────────
// Editing
// ──────────────────────────────────────────────────────────────

void CodeEditor::pushUndo() {
    m_undoStack.push_back({m_lines, m_cursor});
    if (m_undoStack.size() > 100) m_undoStack.erase(m_undoStack.begin());
    m_redoStack.clear();
}

void CodeEditor::InsertChar(char c) {
    pushUndo();
    auto& ln = m_lines[m_cursor.line];
    if (c == '\n') {
        std::string rest = ln.substr(m_cursor.column);
        ln = ln.substr(0, m_cursor.column);
        m_lines.insert(m_lines.begin() + m_cursor.line + 1, rest);
        ++m_cursor.line; m_cursor.column = 0;
    } else {
        ln.insert(m_cursor.column, 1, c);
        ++m_cursor.column;
    }
    m_modified = true;
}

void CodeEditor::InsertText(const std::string& text) {
    for (char c : text) InsertChar(c);
}

void CodeEditor::DeleteChar() {
    pushUndo();
    auto& ln = m_lines[m_cursor.line];
    if (m_cursor.column < (uint32_t)ln.size()) {
        ln.erase(m_cursor.column, 1);
    } else if (m_cursor.line + 1 < (uint32_t)m_lines.size()) {
        ln += m_lines[m_cursor.line + 1];
        m_lines.erase(m_lines.begin() + m_cursor.line + 1);
    }
    m_modified = true;
}

void CodeEditor::DeleteCharBack() {
    if (m_cursor.column > 0) {
        pushUndo();
        m_lines[m_cursor.line].erase(m_cursor.column - 1, 1);
        --m_cursor.column;
        m_modified = true;
    } else if (m_cursor.line > 0) {
        pushUndo();
        uint32_t prevLen = (uint32_t)m_lines[m_cursor.line-1].size();
        m_lines[m_cursor.line-1] += m_lines[m_cursor.line];
        m_lines.erase(m_lines.begin() + m_cursor.line);
        --m_cursor.line;
        m_cursor.column = prevLen;
        m_modified = true;
    }
}

void CodeEditor::DeleteLine(uint32_t idx) {
    if (idx >= (uint32_t)m_lines.size()) return;
    pushUndo();
    m_lines.erase(m_lines.begin() + idx);
    if (m_lines.empty()) m_lines.push_back("");
    clampCursor(); m_modified = true;
}

void CodeEditor::InsertLine(uint32_t idx, const std::string& text) {
    pushUndo();
    m_lines.insert(m_lines.begin() + std::min((size_t)idx, m_lines.size()), text);
    m_modified = true;
}

// ──────────────────────────────────────────────────────────────
// Clipboard
// ──────────────────────────────────────────────────────────────

std::string CodeEditor::Copy() const {
    if (m_selection.IsEmpty()) return GetLine(m_cursor.line);
    // Multi-line selection
    std::string out;
    for (uint32_t l = m_selection.start.line; l <= m_selection.end.line; ++l) {
        uint32_t from = (l == m_selection.start.line) ? m_selection.start.column : 0;
        uint32_t to   = (l == m_selection.end.line)   ? m_selection.end.column
                                                       : (uint32_t)m_lines[l].size();
        if (l > m_selection.start.line) out += '\n';
        out += m_lines[l].substr(from, to - from);
    }
    return out;
}

void CodeEditor::Cut() {
    pushUndo();
    // For simplicity, delete current line if no selection
    if (m_selection.IsEmpty()) DeleteLine(m_cursor.line);
    m_modified = true;
}

void CodeEditor::Paste(const std::string& text) { InsertText(text); }

// ──────────────────────────────────────────────────────────────
// Undo / Redo
// ──────────────────────────────────────────────────────────────

bool CodeEditor::CanUndo() const { return !m_undoStack.empty(); }
bool CodeEditor::CanRedo() const { return !m_redoStack.empty(); }

void CodeEditor::Undo() {
    if (m_undoStack.empty()) return;
    m_redoStack.push_back({m_lines, m_cursor});
    auto& e = m_undoStack.back();
    m_lines = e.lines; m_cursor = e.cursor;
    m_undoStack.pop_back();
    m_modified = true;
}

void CodeEditor::Redo() {
    if (m_redoStack.empty()) return;
    m_undoStack.push_back({m_lines, m_cursor});
    auto& e = m_redoStack.back();
    m_lines = e.lines; m_cursor = e.cursor;
    m_redoStack.pop_back();
    m_modified = true;
}

// ──────────────────────────────────────────────────────────────
// Search
// ──────────────────────────────────────────────────────────────

static std::string toLower(std::string s) {
    for (char& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}

std::vector<EditorCursor> CodeEditor::Find(const std::string& query, bool caseSensitive) const {
    std::vector<EditorCursor> results;
    if (query.empty()) return results;
    for (uint32_t l = 0; l < (uint32_t)m_lines.size(); ++l) {
        std::string hay = caseSensitive ? m_lines[l] : toLower(m_lines[l]);
        std::string ndl = caseSensitive ? query : toLower(query);
        size_t pos = 0;
        while ((pos = hay.find(ndl, pos)) != std::string::npos) {
            results.push_back({l, (uint32_t)pos});
            pos += ndl.size();
        }
    }
    return results;
}

bool CodeEditor::Replace(const std::string& query, const std::string& replacement,
                          bool caseSensitive) {
    auto hits = Find(query, caseSensitive);
    if (hits.empty()) return false;
    pushUndo();
    // Replace first hit after cursor
    for (const auto& h : hits) {
        if (h.line > m_cursor.line || (h.line == m_cursor.line && h.column >= m_cursor.column)) {
            m_lines[h.line].replace(h.column, query.size(), replacement);
            m_cursor = {h.line, h.column + (uint32_t)replacement.size()};
            m_modified = true;
            return true;
        }
    }
    // Wrap to first
    const auto& h = hits.front();
    m_lines[h.line].replace(h.column, query.size(), replacement);
    m_cursor = {h.line, h.column + (uint32_t)replacement.size()};
    m_modified = true;
    return true;
}

int CodeEditor::ReplaceAll(const std::string& query, const std::string& replacement,
                            bool caseSensitive) {
    auto hits = Find(query, caseSensitive);
    if (hits.empty()) return 0;
    pushUndo();
    int count = 0;
    // Replace in reverse order so column offsets stay valid
    for (int i = (int)hits.size()-1; i >= 0; --i) {
        const auto& h = hits[i];
        m_lines[h.line].replace(h.column, query.size(), replacement);
        ++count;
    }
    m_modified = true;
    return count;
}

// ──────────────────────────────────────────────────────────────
// Syntax highlighting — C++ tokeniser
// ──────────────────────────────────────────────────────────────

HighlightedLine CodeEditor::HighlightLine(uint32_t idx) const {
    HighlightedLine hl;
    if (idx >= (uint32_t)m_lines.size()) return hl;
    hl.text = m_lines[idx];
    const std::string& ln = hl.text;
    uint32_t i = 0;
    const uint32_t n = (uint32_t)ln.size();

    while (i < n) {
        // Line comment
        if (i+1 < n && ln[i]=='/' && ln[i+1]=='/') {
            hl.tokens.push_back({i, n, TokenType::Comment});
            break;
        }
        // String literal
        if (ln[i]=='"') {
            uint32_t s = i++;
            while (i < n && ln[i] != '"') { if (ln[i]=='\\') ++i; ++i; }
            if (i < n) ++i;
            hl.tokens.push_back({s, i, TokenType::String});
            continue;
        }
        // Char literal
        if (ln[i]=='\'') {
            uint32_t s = i++;
            while (i < n && ln[i] != '\'') { if (ln[i]=='\\') ++i; ++i; }
            if (i < n) ++i;
            hl.tokens.push_back({s, i, TokenType::String});
            continue;
        }
        // Preprocessor
        if (ln[i]=='#') {
            hl.tokens.push_back({i, n, TokenType::Preprocessor});
            break;
        }
        // Number
        if (std::isdigit((unsigned char)ln[i]) ||
            (ln[i]=='-' && i+1 < n && std::isdigit((unsigned char)ln[i+1]))) {
            uint32_t s = i++;
            while (i < n && (std::isalnum((unsigned char)ln[i]) || ln[i]=='.')) ++i;
            hl.tokens.push_back({s, i, TokenType::Number});
            continue;
        }
        // Identifier or keyword
        if (std::isalpha((unsigned char)ln[i]) || ln[i]=='_') {
            uint32_t s = i;
            while (i < n && (std::isalnum((unsigned char)ln[i]) || ln[i]=='_')) ++i;
            std::string word = ln.substr(s, i-s);
            TokenType tt = TokenType::Identifier;
            if (m_language=="cpp") {
                if (IsCppKeyword(word)) tt = TokenType::Keyword;
                else if (IsCppType(word)) tt = TokenType::Type;
            }
            hl.tokens.push_back({s, i, tt});
            continue;
        }
        // Operator
        if (std::ispunct((unsigned char)ln[i]) && ln[i]!='_') {
            hl.tokens.push_back({i, i+1,
                (ln[i]=='('||ln[i]==')'||ln[i]=='{'||ln[i]=='}'||ln[i]==';'||ln[i]==',')
                ? TokenType::Punctuation : TokenType::Operator});
        }
        ++i;
    }
    return hl;
}

std::vector<HighlightedLine> CodeEditor::HighlightAll() const {
    std::vector<HighlightedLine> out;
    out.reserve(m_lines.size());
    for (uint32_t i = 0; i < (uint32_t)m_lines.size(); ++i)
        out.push_back(HighlightLine(i));
    return out;
}

// ──────────────────────────────────────────────────────────────
// Breakpoints
// ──────────────────────────────────────────────────────────────

bool CodeEditor::HasBreakpoint(uint32_t line) const {
    return std::any_of(m_breakpoints.begin(), m_breakpoints.end(),
        [line](const Breakpoint& b){ return b.line == line && b.enabled; });
}

void CodeEditor::ToggleBreakpoint(uint32_t line) {
    for (auto it = m_breakpoints.begin(); it != m_breakpoints.end(); ++it) {
        if (it->line == line) { m_breakpoints.erase(it); return; }
    }
    m_breakpoints.push_back({line, m_filePath, true, ""});
}

void CodeEditor::AddBreakpoint(const Breakpoint& bp) {
    if (!HasBreakpoint(bp.line)) m_breakpoints.push_back(bp);
}

void CodeEditor::RemoveBreakpoint(uint32_t line) {
    m_breakpoints.erase(std::remove_if(m_breakpoints.begin(), m_breakpoints.end(),
        [line](const Breakpoint& b){ return b.line == line; }), m_breakpoints.end());
}

// ──────────────────────────────────────────────────────────────
// AI completion
// ──────────────────────────────────────────────────────────────

void CodeEditor::SetCompletionProvider(CompletionProvider fn) {
    m_completionFn = std::move(fn);
}

std::vector<CompletionItem> CodeEditor::GetCompletions() const {
    if (!m_completionFn) return {};
    // Extract prefix at cursor
    const auto& ln = GetLine(m_cursor.line);
    uint32_t col = std::min(m_cursor.column, (uint32_t)ln.size());
    uint32_t start = col;
    while (start > 0 && (std::isalnum((unsigned char)ln[start-1]) || ln[start-1]=='_'))
        --start;
    std::string prefix = ln.substr(start, col - start);
    // Context = surrounding lines
    std::ostringstream ctx;
    uint32_t lo = (m_cursor.line > 3) ? m_cursor.line - 3 : 0;
    for (uint32_t l = lo; l <= m_cursor.line; ++l)
        ctx << m_lines[l] << '\n';
    return m_completionFn(prefix, ctx.str());
}

} // namespace IDE
