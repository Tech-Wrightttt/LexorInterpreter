#include "Parser.h"
#include <stdexcept>

Parser::Parser(std::vector<Token> toks) : tokens(std::move(toks)) {}

// ── Token navigation ─────────────────────────────────────────────────────────

bool  Parser::atEnd()  const { return cur >= tokens.size() || tokens[cur].type == TokenType::END_OF_FILE; }
Token& Parser::peek()        { return tokens[cur]; }
Token& Parser::previous()    { return tokens[cur - 1]; }

Token Parser::advance() {
    if (!atEnd()) cur++;
    return tokens[cur - 1];
}

bool Parser::check(TokenType type) const {
    return !atEnd() && tokens[cur].type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) { advance(); return true; }
    return false;
}

Token Parser::expect(TokenType type, const std::string& msg) {
    if (check(type)) return advance();
    Token& t = peek();
    throw std::runtime_error("Parse error at line " + std::to_string(t.line) +
                             ", col " + std::to_string(t.col) + ": " + msg);
}

void Parser::skipNewlines() {
    while (check(TokenType::NEWLINE)) advance();
}

// ── Public entry ─────────────────────────────────────────────────────────────

std::vector<StmtPtr> Parser::parse() {
    std::vector<StmtPtr> program;

    // Consume everything up to START SCRIPT
    while (!atEnd() && !check(TokenType::START_SCRIPT)) advance();
    if (!atEnd()) advance(); // consume START SCRIPT

    skipNewlines();

    while (!atEnd() && !check(TokenType::END_SCRIPT)) {
        if (match(TokenType::NEWLINE)) continue;
        program.push_back(parseStatement());
    }
    return program;
}

// ── Statement dispatch ────────────────────────────────────────────────────────

StmtPtr Parser::parseStatement() {
    if (check(TokenType::DECLARE)) return parseDeclare();
    if (check(TokenType::PRINT))   return parsePrint();
    return parseAssignOrExpr();
}

// DECLARE <type> name [= expr] [, name [= expr]] ...
StmtPtr Parser::parseDeclare() {
    advance(); // consume DECLARE

    // Consume type keyword
    TokenType varType = peek().type;
    if (varType != TokenType::INT_TYPE  && varType != TokenType::CHAR_TYPE &&
        varType != TokenType::BOOL_TYPE && varType != TokenType::FLOAT_TYPE)
        throw std::runtime_error("Expected type keyword after DECLARE");
    advance();

    auto stmt = std::make_unique<Stmt>();
    DeclareStmt decl;
    decl.varType = varType;

    do {
        if (match(TokenType::COMMA)) continue;
        if (!check(TokenType::IDENTIFIER)) break;
        std::string name = advance().value;
        ExprPtr init = nullptr;
        if (match(TokenType::ASSIGN)) init = parseExpr();
        decl.vars.emplace_back(name, std::move(init));
    } while (!atEnd() && !check(TokenType::NEWLINE));

    stmt->data = std::move(decl);
    return stmt;
}

// identifier [= ...] — handles chained assignment
StmtPtr Parser::parseAssignOrExpr() {
    auto stmt = std::make_unique<Stmt>();
    AssignStmt assign;

    // Collect all "name =" prefixes
    while (check(TokenType::IDENTIFIER) &&
           cur + 1 < tokens.size() && tokens[cur + 1].type == TokenType::ASSIGN) {
        assign.targets.push_back(advance().value); // name
        advance();                                  // '='
    }

    assign.value = parseExpr();
    stmt->data = std::move(assign);
    return stmt;
}

// PRINT: part part part ...
StmtPtr Parser::parsePrint() {
    advance(); // consume PRINT
    match(TokenType::COLON);

    auto stmt = std::make_unique<Stmt>();
    PrintStmt ps;
    ps.parts = parsePrintParts();
    stmt->data = std::move(ps);
    return stmt;
}

// ── Expression parsers ────────────────────────────────────────────────────────

std::vector<ExprPtr> Parser::parsePrintParts() {
    std::vector<ExprPtr> parts;
    while (!atEnd() && !check(TokenType::NEWLINE) && !check(TokenType::END_SCRIPT)) {
        if (check(TokenType::LBRACKET)) {
            parts.push_back(parseBracketGroup());
        } else if (check(TokenType::DOLLAR)) {
            advance();
            parts.push_back(makeLiteral("\n"));
        } else if (check(TokenType::AMPERSAND)) {
            advance(); // separator consumed but not printed
        } else {
            parts.push_back(parsePrimary());
        }
    }
    return parts;
}

// Bracket group: [ parts... ]
ExprPtr Parser::parseBracketGroup() {
    expect(TokenType::LBRACKET, "Expected '['");
    std::vector<ExprPtr> inner;

    // Depth-tracked to handle nested brackets
    int depth = 1;
    while (!atEnd() && depth > 0) {
        if (check(TokenType::LBRACKET)) {
            advance(); depth++;
            // Represent nested bracket as its own BracketExpr
            // (we re-enter by collecting a sub-group)
            // For simplicity push a literal "[" so inner brackets print verbatim
            inner.push_back(makeLiteral("["));
        } else if (check(TokenType::RBRACKET)) {
            advance(); depth--;
            if (depth > 0) inner.push_back(makeLiteral("]"));
        } else if (check(TokenType::DOLLAR)) {
            advance();
            inner.push_back(makeLiteral("\n"));
        } else if (check(TokenType::AMPERSAND)) {
            advance();
        } else {
            inner.push_back(parsePrimary());
        }
    }
    return makeBracket(std::move(inner));
}

ExprPtr Parser::parseExpr() {
    return parsePrimary(); // extend here for BinaryExpr / math later
}

ExprPtr Parser::parsePrimary() {
    Token t = advance();
    switch (t.type) {
        case TokenType::IDENTIFIER: return makeVar(t.value);
        case TokenType::INT_LIT:
        case TokenType::FLOAT_LIT:
        case TokenType::BOOL_LIT:
        case TokenType::STRING_LIT:
        case TokenType::CHAR_LIT:   return makeLiteral(t.value);
        default:
            return makeLiteral(t.value); // fallback
    }
}
