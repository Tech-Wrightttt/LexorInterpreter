#pragma once
#include <string>
#include <memory>
#include <vector>
#include "../Lexer/Token.h"
#include "Expr.h"

// ── Forward declaration ──────────────────────────────────────────────────────
struct Stmt;
using StmtPtr = std::unique_ptr<Stmt>;

// ── Node kinds ───────────────────────────────────────────────────────────────

// DECLARE INT x = 5, y
struct DeclareStmt {
    TokenType               varType; // INT_TYPE | CHAR_TYPE | BOOL_TYPE | FLOAT_TYPE
    std::vector<std::pair<std::string, ExprPtr>> vars; // name -> optional init expr (nullptr = no init)
};

// x = expr  (also handles chained: a = b = expr)
struct AssignStmt {
    std::vector<std::string> targets; // all lhs names, left to right
    ExprPtr                  value;
};

// PRINT: [expr] & $
struct PrintStmt {
    std::vector<ExprPtr> parts; // each part is evaluated and printed in order
};

// Placeholder for future: IF condition block
struct IfStmt {
    ExprPtr              condition;
    std::vector<StmtPtr> body;
};

// SCAN: var, var, ...
struct ScanStmt {
    std::vector<std::string> targets; // variable names to read into
};

// ── Variant node ─────────────────────────────────────────────────────────────
#include <variant>

struct Stmt {
    std::variant<DeclareStmt, AssignStmt, PrintStmt, IfStmt, ScanStmt> data;
};
