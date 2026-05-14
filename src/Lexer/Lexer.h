#pragma once
#include <string>
#include "Token.h"

class Lexer {
public:
    explicit Lexer(std::string source);

    Token nextToken();

private:
    std::string src;
    size_t      pos  = 0;
    int         line = 1;
    int         col  = 1;

    void        advance();
    char        current() const;
    bool        atEnd()   const;

    Token makeToken(TokenType type, const std::string& value) const;
    Token lexIdentifierOrKeyword();
    Token lexNumber();
    Token lexString();
};
