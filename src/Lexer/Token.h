#pragma once
#include <string>

enum class TokenType {
    // Structure
    SCRIPT_AREA, START_SCRIPT, END_SCRIPT,
    // Keywords
    DECLARE, INT_TYPE, CHAR_TYPE, BOOL_TYPE, FLOAT_TYPE,
    PRINT, SCAN,
    // Literals & Identifiers
    IDENTIFIER, INT_LIT, FLOAT_LIT, CHAR_LIT, BOOL_LIT, STRING_LIT,
    // Symbols
    ASSIGN, AMPERSAND, DOLLAR, LBRACKET, RBRACKET, HASH, COLON, COMMA,
    // Control
    NEWLINE, END_OF_FILE,
    // Operators 
    PLUS, MINUS, STAR, SLASH, PERCENT, LPAREN, RPAREN,
    // also for comparisons:
    GT, LT, GTE, LTE, EQ, NEQ,
    // Logical operators 
    AND, OR, NOT
};

struct Token {
    TokenType   type;
    std::string value;
    int         line;
    int         col;
};
