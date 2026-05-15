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

    // 1. Require SCRIPT AREA
    skipNewlines();
    if (!check(TokenType::SCRIPT_AREA))
        throw std::runtime_error("Expected 'SCRIPT AREA' at the start of the program");
    advance();

    // 2. Require START SCRIPT (and only one)
    skipNewlines();
    if (!check(TokenType::START_SCRIPT))
        throw std::runtime_error("Expected 'START SCRIPT' after 'SCRIPT AREA'");
    advance();

    skipNewlines();

    // 3. Parse body — track if declarations section is over
    bool declarationsDone = false;
    while (!atEnd() && !check(TokenType::END_SCRIPT)) {
        if (match(TokenType::NEWLINE)) continue;

        // Reject a second START SCRIPT
        if (check(TokenType::START_SCRIPT))
            throw std::runtime_error("Unexpected 'START SCRIPT': already inside a script block");

        // Enforce: DECLARE must come before executable statements
        if (check(TokenType::DECLARE)) {
            if (declarationsDone)
                throw std::runtime_error(
                    "Line " + std::to_string(peek().line) +
                    ": DECLARE statements must appear before any executable statements");
            program.push_back(parseDeclare());
        } else {
            declarationsDone = true;
            program.push_back(parseStatement());
        }
    }

    // 4. Require END SCRIPT
    if (!check(TokenType::END_SCRIPT))
        throw std::runtime_error("Expected 'END SCRIPT' to close the script block");
    advance();

    // 5. Reject anything after END SCRIPT
    skipNewlines();
    if (!atEnd())
        throw std::runtime_error(
            "Line " + std::to_string(peek().line) +
            ": unexpected tokens after 'END SCRIPT'");

    return program;
}

// ── Statement dispatch ────────────────────────────────────────────────────────

StmtPtr Parser::parseStatement() {
    if (check(TokenType::PRINT)) return parsePrint();

    // Catch reserved words used where a statement is expected
    TokenType t = peek().type;
    if (t == TokenType::INT_TYPE  || t == TokenType::CHAR_TYPE ||
        t == TokenType::BOOL_TYPE || t == TokenType::FLOAT_TYPE ||
        t == TokenType::END_SCRIPT || t == TokenType::SCRIPT_AREA) {
        throw std::runtime_error(
            "Line " + std::to_string(peek().line) +
            ": unexpected reserved word '" + peek().value + "'");
    }

    if (check(TokenType::SCAN)) return parseScan();
    return parseAssignOrExpr();
}

// DECLARE <type> name [= expr] [, name [= expr]] ...
StmtPtr Parser::parseDeclare() {
    advance(); // consume DECLARE

    // Require a valid type keyword
    TokenType varType = peek().type;
    if (varType != TokenType::INT_TYPE  && varType != TokenType::CHAR_TYPE &&
        varType != TokenType::BOOL_TYPE && varType != TokenType::FLOAT_TYPE)
        throw std::runtime_error(
            "Line " + std::to_string(peek().line) +
            ": expected a type (INT, FLOAT, CHAR, BOOL) after DECLARE");
    advance();

    auto stmt = std::make_unique<Stmt>();
    DeclareStmt decl;
    decl.varType = varType;

    bool first = true;
    while (!atEnd() && !check(TokenType::NEWLINE)) {
        // Expect comma between variables (not before the first one)
        if (!first) {
            if (!check(TokenType::COMMA))
                throw std::runtime_error(
                    "Line " + std::to_string(peek().line) +
                    ": expected ',' between variable declarations");
            advance(); // consume comma
        }
        first = false;

        // Must be an IDENTIFIER — reject reserved words used as names
        if (!check(TokenType::IDENTIFIER)) {
            throw std::runtime_error(
                "Line " + std::to_string(peek().line) +
                ": '" + peek().value + "' is a reserved word and cannot be used as a variable name");
        }

        std::string name = advance().value;
        ExprPtr init = nullptr;
        if (match(TokenType::ASSIGN))
            init = parseExpr();

        decl.vars.emplace_back(name, std::move(init));
    }

    if (decl.vars.empty())
        throw std::runtime_error("DECLARE statement has no variable names");

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
    expect(TokenType::COLON, "Expected ':' after PRINT");

    auto stmt = std::make_unique<Stmt>();
    PrintStmt ps;
    ps.parts = parsePrintParts();
    stmt->data = std::move(ps);
    return stmt;
}

StmtPtr Parser::parseScan() {
    advance(); // consume SCAN
    expect(TokenType::COLON, "Expected ':' after SCAN");

    auto stmt = std::make_unique<Stmt>();
    ScanStmt ss;

    // Expect at least one variable name
    Token nameTok = expect(TokenType::IDENTIFIER, "Expected variable name after 'SCAN:'");
    ss.targets.push_back(nameTok.value);

    while (match(TokenType::COMMA)) {
        Token next = expect(TokenType::IDENTIFIER, "Expected variable name after ','");
        ss.targets.push_back(next.value);
    }

    stmt->data = std::move(ss);
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
            parts.push_back(parseExpr());
        }
    }
    return parts;
}

// Bracket group: [ parts... ]
ExprPtr Parser::parseBracketGroup() {
    expect(TokenType::LBRACKET, "Expected '['");
    std::string literal;

    while (!atEnd() && !check(TokenType::NEWLINE)) {
        if (check(TokenType::LBRACKET)) {
            // [[ = escaped [
            advance();
            literal += '[';
        } else if (check(TokenType::RBRACKET)) {
            // single ] = closing bracket
            advance();
            break;
        } else {
            literal += peek().value;
            advance();
        }
    }

    return makeLiteral(literal);
}

// Precedence (lowest to highest):
// OR
// AND
// NOT
// > < >= <= == <>
// + -
// * / %
// unary + -
// primary

ExprPtr Parser::parseExpr() {
    return parseOr();
}

ExprPtr Parser::parseOr() {
    ExprPtr left = parseAnd();
    while (check(TokenType::OR)) {
        advance();
        ExprPtr right = parseAnd();
        left = makeBinary("OR", std::move(left), std::move(right));
    }
    return left;
}

ExprPtr Parser::parseAnd() {
    ExprPtr left = parseNot();
    while (check(TokenType::AND)) {
        advance();
        ExprPtr right = parseNot();
        left = makeBinary("AND", std::move(left), std::move(right));
    }
    return left;
}

ExprPtr Parser::parseNot() {
    if (check(TokenType::NOT)) {
        advance();
        ExprPtr operand = parseNot(); // right-associative
        return makeUnary("NOT", std::move(operand));
    }
    return parseComparison();
}

ExprPtr Parser::parseComparison() {
    ExprPtr left = parseAddSub();
    while (check(TokenType::GT)  || check(TokenType::LT) ||
           check(TokenType::GTE) || check(TokenType::LTE) ||
           check(TokenType::EQ)  || check(TokenType::NEQ)) {
        std::string op = peek().value;
        advance();
        ExprPtr right = parseAddSub();
        left = makeBinary(op, std::move(left), std::move(right));
    }
    return left;
}

ExprPtr Parser::parseAddSub() {
    ExprPtr left = parseMulDiv();
    while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
        std::string op = peek().value;
        advance();
        ExprPtr right = parseMulDiv();
        left = makeBinary(op, std::move(left), std::move(right));
    }
    return left;
}

ExprPtr Parser::parseMulDiv() {
    ExprPtr left = parseUnary();
    while (check(TokenType::STAR) || check(TokenType::SLASH) || check(TokenType::PERCENT)) {
        std::string op = peek().value;
        advance();
        ExprPtr right = parseUnary();
        left = makeBinary(op, std::move(left), std::move(right));
    }
    return left;
}

ExprPtr Parser::parseUnary() {
    if (check(TokenType::MINUS)) {
        advance();
        ExprPtr operand = parseUnary();
        return makeUnary("-", std::move(operand));
    }
    if (check(TokenType::PLUS)) {
        advance();
        return parseUnary(); // unary + is a no-op
    }
    return parsePrimary();
}

ExprPtr Parser::parsePrimary() {
    if (check(TokenType::LPAREN)) {
        advance();                    // consume '('
        ExprPtr expr = parseExpr();   // parse inner expression
        expect(TokenType::RPAREN, "Expected ')' after expression");
        return expr;
    }

    Token t = advance();
    switch (t.type) {
        case TokenType::IDENTIFIER: return makeVar(t.value);
        case TokenType::INT_LIT:
        case TokenType::FLOAT_LIT:
        case TokenType::BOOL_LIT:
        case TokenType::STRING_LIT:
        case TokenType::CHAR_LIT:   return makeLiteral(t.value);
        default:
            return makeLiteral(t.value);
    }
}
