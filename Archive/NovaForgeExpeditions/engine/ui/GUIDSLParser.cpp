#include "GUIDSLParser.h"
#include <charconv>
#include <stdexcept>

namespace atlas::ui {

void GUIDSLParser::SkipWhitespace() {
    while (m_pos < m_source.size()) {
        char c = m_source[m_pos];
        if (c == '\n') {
            ++m_line;
            ++m_pos;
        } else if (c == ' ' || c == '\t' || c == '\r') {
            ++m_pos;
        } else if (c == '#') {
            while (m_pos < m_source.size() && m_source[m_pos] != '\n')
                ++m_pos;
        } else {
            break;
        }
    }
}

GUIDSLParser::Token GUIDSLParser::NextToken() {
    SkipWhitespace();

    if (m_pos >= m_source.size())
        return {Token::Eof, "", m_line};

    char c = m_source[m_pos];

    if (c == '{') { ++m_pos; return {Token::LBrace, "{", m_line}; }
    if (c == '}') { ++m_pos; return {Token::RBrace, "}", m_line}; }

    if (c == '"') {
        ++m_pos;
        std::string val;
        while (m_pos < m_source.size() && m_source[m_pos] != '"') {
            if (m_source[m_pos] == '\\' && m_pos + 1 < m_source.size()) {
                ++m_pos;
                val += m_source[m_pos];
            } else {
                if (m_source[m_pos] == '\n') ++m_line;
                val += m_source[m_pos];
            }
            ++m_pos;
        }
        if (m_pos >= m_source.size())
            return {Token::Error, "unterminated string literal", m_line};
        ++m_pos; // closing quote
        return {Token::String, val, m_line};
    }

    if (std::isdigit(static_cast<unsigned char>(c)) ||
        (c == '.' && m_pos + 1 < m_source.size() && std::isdigit(static_cast<unsigned char>(m_source[m_pos + 1])))) {
        size_t start = m_pos;
        bool hasDot = (c == '.');
        ++m_pos;
        while (m_pos < m_source.size()) {
            char d = m_source[m_pos];
            if (std::isdigit(static_cast<unsigned char>(d))) {
                ++m_pos;
            } else if (d == '.' && !hasDot) {
                hasDot = true;
                ++m_pos;
            } else {
                break;
            }
        }
        return {Token::Number, m_source.substr(start, m_pos - start), m_line};
    }

    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
        size_t start = m_pos;
        ++m_pos;
        while (m_pos < m_source.size() &&
               (std::isalnum(static_cast<unsigned char>(m_source[m_pos])) || m_source[m_pos] == '_'))
            ++m_pos;
        return {Token::Identifier, m_source.substr(start, m_pos - start), m_line};
    }

    ++m_pos;
    return {Token::Error, std::string("unexpected character '") + c + "'", m_line};
}

GUIDSLParser::Token GUIDSLParser::PeekToken() {
    size_t savedPos = m_pos;
    int savedLine = m_line;
    Token tok = NextToken();
    m_pos = savedPos;
    m_line = savedLine;
    return tok;
}

DSLParseResult GUIDSLParser::Parse(const std::string& source) {
    m_source = source;
    m_pos = 0;
    m_line = 1;

    DSLParseResult result;

    try {
        result.root = ParseLayout();
        Token eof = NextToken();
        if (eof.type != Token::Eof) {
            result.success = false;
            result.errorMessage = "unexpected token '" + eof.value + "' after layout block";
            result.errorLine = eof.line;
            result.root.reset();
            return result;
        }
        result.success = true;
    } catch (const std::runtime_error& e) {
        result.success = false;
        result.errorMessage = e.what();
        result.errorLine = m_line;
        result.root.reset();
    }

    return result;
}

std::unique_ptr<DSLNode> GUIDSLParser::ParseLayout() {
    Token tok = NextToken();
    if (tok.type != Token::Identifier || tok.value != "layout")
        throw std::runtime_error("expected 'layout' at line " + std::to_string(tok.line));

    Token nameTok = NextToken();
    if (nameTok.type != Token::String)
        throw std::runtime_error("expected layout name string at line " + std::to_string(nameTok.line));

    auto node = std::make_unique<DSLNode>();
    node->type = DSLNodeType::Layout;
    node->name = nameTok.value;

    node->children = ParseBlock();
    return node;
}

std::vector<std::unique_ptr<DSLNode>> GUIDSLParser::ParseBlock() {
    Token lbrace = NextToken();
    if (lbrace.type != Token::LBrace)
        throw std::runtime_error("expected '{' at line " + std::to_string(lbrace.line));

    std::vector<std::unique_ptr<DSLNode>> children;
    while (true) {
        Token peek = PeekToken();
        if (peek.type == Token::RBrace)
            break;
        if (peek.type == Token::Eof)
            throw std::runtime_error("unexpected end of input, expected '}' at line " + std::to_string(peek.line));
        children.push_back(ParseStatement());
    }

    Token rbrace = NextToken();
    if (rbrace.type != Token::RBrace)
        throw std::runtime_error("expected '}' at line " + std::to_string(rbrace.line));

    return children;
}

std::unique_ptr<DSLNode> GUIDSLParser::ParseStatement() {
    Token peek = PeekToken();
    if (peek.type != Token::Identifier)
        throw std::runtime_error("expected statement at line " + std::to_string(peek.line));

    if (peek.value == "split")  return ParseSplit();
    if (peek.value == "panel")  return ParsePanel();
    if (peek.value == "dock")   return ParseDock();
    if (peek.value == "tabs")   return ParseTabs();

    throw std::runtime_error("unknown statement '" + peek.value + "' at line " + std::to_string(peek.line));
}

std::unique_ptr<DSLNode> GUIDSLParser::ParseSplit() {
    Token tok = NextToken(); // consume "split"

    Token dirTok = NextToken();
    if (dirTok.type != Token::Identifier ||
        (dirTok.value != "horizontal" && dirTok.value != "vertical"))
        throw std::runtime_error("expected 'horizontal' or 'vertical' after 'split' at line " + std::to_string(dirTok.line));

    Token ratioTok = NextToken();
    if (ratioTok.type != Token::Number)
        throw std::runtime_error("expected split ratio number at line " + std::to_string(ratioTok.line));

    float ratio = 0.5f;
    try { ratio = std::stof(ratioTok.value); }
    catch (const std::invalid_argument&) { throw std::runtime_error("invalid split ratio '" + ratioTok.value + "' at line " + std::to_string(ratioTok.line)); }
    catch (const std::out_of_range&) { throw std::runtime_error("split ratio out of range '" + ratioTok.value + "' at line " + std::to_string(ratioTok.line)); }

    auto node = std::make_unique<DSLNode>();
    node->type = DSLNodeType::Split;
    node->splitDir = (dirTok.value == "horizontal") ? DSLSplitDir::Horizontal : DSLSplitDir::Vertical;
    node->ratio = ratio;

    node->children = ParseBlock();
    return node;
}

std::unique_ptr<DSLNode> GUIDSLParser::ParsePanel() {
    Token tok = NextToken(); // consume "panel"

    Token nameTok = NextToken();
    if (nameTok.type != Token::String)
        throw std::runtime_error("expected panel name string at line " + std::to_string(nameTok.line));

    auto node = std::make_unique<DSLNode>();
    node->type = DSLNodeType::Panel;
    node->name = nameTok.value;
    return node;
}

std::unique_ptr<DSLNode> GUIDSLParser::ParseDock() {
    Token tok = NextToken(); // consume "dock"

    Token targetTok = NextToken();
    if (targetTok.type != Token::Identifier)
        throw std::runtime_error("expected dock target at line " + std::to_string(targetTok.line));

    DSLDockTarget target;
    if      (targetTok.value == "top")    target = DSLDockTarget::Top;
    else if (targetTok.value == "bottom") target = DSLDockTarget::Bottom;
    else if (targetTok.value == "left")   target = DSLDockTarget::Left;
    else if (targetTok.value == "right")  target = DSLDockTarget::Right;
    else if (targetTok.value == "center") target = DSLDockTarget::Center;
    else if (targetTok.value == "float")  target = DSLDockTarget::Float;
    else throw std::runtime_error("unknown dock target '" + targetTok.value + "' at line " + std::to_string(targetTok.line));

    Token ratioTok = NextToken();
    if (ratioTok.type != Token::Number)
        throw std::runtime_error("expected dock ratio number at line " + std::to_string(ratioTok.line));

    float ratio = 0.5f;
    try { ratio = std::stof(ratioTok.value); }
    catch (const std::invalid_argument&) { throw std::runtime_error("invalid dock ratio '" + ratioTok.value + "' at line " + std::to_string(ratioTok.line)); }
    catch (const std::out_of_range&) { throw std::runtime_error("dock ratio out of range '" + ratioTok.value + "' at line " + std::to_string(ratioTok.line)); }

    auto node = std::make_unique<DSLNode>();
    node->type = DSLNodeType::Dock;
    node->dockTarget = target;
    node->ratio = ratio;

    node->children = ParseBlock();
    return node;
}

std::unique_ptr<DSLNode> GUIDSLParser::ParseTabs() {
    Token tok = NextToken(); // consume "tabs"

    auto node = std::make_unique<DSLNode>();
    node->type = DSLNodeType::Tabs;

    Token lbrace = NextToken();
    if (lbrace.type != Token::LBrace)
        throw std::runtime_error("expected '{' after 'tabs' at line " + std::to_string(lbrace.line));

    while (true) {
        Token peek = PeekToken();
        if (peek.type == Token::RBrace)
            break;
        if (peek.type == Token::Eof)
            throw std::runtime_error("unexpected end of input in tabs block at line " + std::to_string(peek.line));
        if (peek.type != Token::Identifier || peek.value != "panel")
            throw std::runtime_error("only 'panel' statements are allowed inside 'tabs' at line " + std::to_string(peek.line));
        node->children.push_back(ParsePanel());
    }

    Token rbrace = NextToken();
    if (rbrace.type != Token::RBrace)
        throw std::runtime_error("expected '}' at line " + std::to_string(rbrace.line));

    return node;
}

} // namespace atlas::ui
