#include "Lexer.h"
#include <cctype>
#include <stdexcept>

Lexer::Lexer(std::string source) : src(std::move(source)) {}

bool  Lexer::atEnd()   const { return pos >= src.length(); }
char  Lexer::current() const { return atEnd() ? '\0' : src[pos]; }

void Lexer::advance() {
    if (!atEnd()) {
        if (src[pos] == '\n') { line++; col = 1; }
        else                  { col++; }
        pos++;
    }
}

Token Lexer::makeToken(TokenType type, const std::string& value) const {
    return {type, value, line, col};
}

// ── Public ──────────────────────────────────────────────────────────────────

Token Lexer::nextToken() {
    // Skip non-newline whitespace
    while (!atEnd() && isspace((unsigned char)current()) && current() != '\n')
        advance();

    if (atEnd()) return makeToken(TokenType::END_OF_FILE, "");

    char c = current();

    // Newline
    if (c == '\n') { advance(); return makeToken(TokenType::NEWLINE, "\n"); }

    // Comments: %% to end of line
    if (c == '%' && pos + 1 < src.length() && src[pos + 1] == '%') {
        while (!atEnd() && current() != '\n') advance();
        return nextToken(); // tail-recurse to get the next real token
    }

    if (isalpha((unsigned char)c) || c == '_') return lexIdentifierOrKeyword();
    if (isdigit((unsigned char)c))             return lexNumber();
    if (c == '"' || c == '\'')                 return lexString();

    // Single-character symbols
    int startCol = col;
    advance();
    switch (c) {
        case '=': return {TokenType::ASSIGN,    "=", line, startCol};
        case ':': return {TokenType::COLON,     ":", line, startCol};
        case '&': return {TokenType::AMPERSAND, "&", line, startCol};
        case '$': return {TokenType::DOLLAR,    "$", line, startCol};
        case '[': return {TokenType::LBRACKET,  "[", line, startCol};
        case ']': return {TokenType::RBRACKET,  "]", line, startCol};
        case '#': return {TokenType::HASH,      "#", line, startCol};
        case ',': return {TokenType::COMMA,     ",", line, startCol};
        default:  return nextToken(); // skip unknown characters
    }
}

// ── Private helpers ──────────────────────────────────────────────────────────

Token Lexer::lexIdentifierOrKeyword() {
    int startCol = col;
    std::string val;
    while (!atEnd() && (isalnum((unsigned char)current()) || current() == '_'))
        { val += current(); advance(); }

    // Two-word keywords: peek ahead past whitespace
    auto peekKeyword = [&](const std::string& second, TokenType type) -> bool {
        size_t t = pos;
        while (t < src.length() && isspace((unsigned char)src[t]) && src[t] != '\n') t++;
        if (src.substr(t, second.size()) == second) {
            // consume the whitespace and the second word
            while (pos < t) advance();
            for (size_t i = 0; i < second.size(); i++) advance();
            return true;
        }
        return false;
    };

    if (val == "SCRIPT" && peekKeyword("AREA",   TokenType::SCRIPT_AREA))
        return {TokenType::SCRIPT_AREA,   "SCRIPT AREA",   line, startCol};
    if (val == "START"  && peekKeyword("SCRIPT", TokenType::START_SCRIPT))
        return {TokenType::START_SCRIPT,  "START SCRIPT",  line, startCol};
    if (val == "END"    && peekKeyword("SCRIPT", TokenType::END_SCRIPT))
        return {TokenType::END_SCRIPT,    "END SCRIPT",    line, startCol};

    if (val == "DECLARE") return {TokenType::DECLARE,   "DECLARE", line, startCol};
    if (val == "INT")     return {TokenType::INT_TYPE,  "INT",     line, startCol};
    if (val == "CHAR")    return {TokenType::CHAR_TYPE, "CHAR",    line, startCol};
    if (val == "BOOL")    return {TokenType::BOOL_TYPE, "BOOL",    line, startCol};
    if (val == "FLOAT")   return {TokenType::FLOAT_TYPE,"FLOAT",   line, startCol};
    if (val == "PRINT")   return {TokenType::PRINT,     "PRINT",   line, startCol};

    return {TokenType::IDENTIFIER, val, line, startCol};
}

Token Lexer::lexNumber() {
    int startCol = col;
    std::string val;
    while (!atEnd() && isdigit((unsigned char)current())) { val += current(); advance(); }
    if (!atEnd() && current() == '.') {
        val += current(); advance();
        while (!atEnd() && isdigit((unsigned char)current())) { val += current(); advance(); }
        return {TokenType::FLOAT_LIT, val, line, startCol};
    }
    return {TokenType::INT_LIT, val, line, startCol};
}

Token Lexer::lexString() {
    int startCol = col;
    char quote = current();
    advance(); // consume opening quote
    std::string val;
    while (!atEnd() && current() != quote) { val += current(); advance(); }
    if (!atEnd()) advance(); // consume closing quote

    if (val == "TRUE" || val == "FALSE")
        return {TokenType::BOOL_LIT, val, line, startCol};
    return {TokenType::STRING_LIT, val, line, startCol};
}
