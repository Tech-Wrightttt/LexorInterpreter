#pragma once
#include <string>
#include <memory>
#include <vector>

// ── Forward declaration ──────────────────────────────────────────────────────
struct Expr;
using ExprPtr = std::unique_ptr<Expr>;

// ── Node kinds ───────────────────────────────────────────────────────────────

struct LiteralExpr {
    std::string value; // raw literal text (already stripped of quotes by Lexer)
};

struct VarExpr {
    std::string name;
};

// Placeholder for future math support: left OP right
struct BinaryExpr {
    ExprPtr     left;
    char        op;   // '+', '-', '*', '/'
    ExprPtr     right;
};

// A bracket-delimited group in a PRINT: [ ... ]
struct BracketExpr {
    std::vector<ExprPtr> parts;
};

// ── Variant node ─────────────────────────────────────────────────────────────
#include <variant>

struct Expr {
    std::variant<LiteralExpr, VarExpr, BinaryExpr, BracketExpr> data;
};

// ── Convenience factories ─────────────────────────────────────────────────────
inline ExprPtr makeLiteral(std::string v) {
    auto e = std::make_unique<Expr>();
    e->data = LiteralExpr{std::move(v)};
    return e;
}
inline ExprPtr makeVar(std::string name) {
    auto e = std::make_unique<Expr>();
    e->data = VarExpr{std::move(name)};
    return e;
}
inline ExprPtr makeBracket(std::vector<ExprPtr> parts) {
    auto e = std::make_unique<Expr>();
    e->data = BracketExpr{std::move(parts)};
    return e;
}
