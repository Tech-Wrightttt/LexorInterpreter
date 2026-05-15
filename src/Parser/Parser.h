#pragma once
#include <vector>
#include <memory>
#include "../Lexer/Token.h"
#include "../AST/Stmt.h"
#include "../AST/Expr.h"

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    // Returns the full list of top-level statements inside START SCRIPT … END SCRIPT
    std::vector<StmtPtr> parse();

private:
    std::vector<Token> tokens;
    size_t             cur = 0;

    // ── Token navigation ───────────────────────────────────────────────────
    Token&       peek();
    Token&       previous();
    Token        advance();
    bool         check(TokenType type) const;
    bool         match(TokenType type);
    Token        expect(TokenType type, const std::string& msg);
    bool         atEnd() const;
    void         skipNewlines();

    // ── Statement parsers ──────────────────────────────────────────────────
    StmtPtr parseStatement();
    StmtPtr parseDeclare();
    StmtPtr parseAssignOrExpr();
    StmtPtr parsePrint();
    StmtPtr parseScan();

    // ── Expression parsers ─────────────────────────────────────────────────
    ExprPtr parseExpr();
    ExprPtr parsePrimary();
    ExprPtr parseBracketGroup();
    std::vector<ExprPtr> parsePrintParts();
};
