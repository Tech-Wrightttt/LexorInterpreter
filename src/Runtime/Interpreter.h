#pragma once
#include <vector>
#include "../AST/Stmt.h"
#include "../AST/Expr.h"
#include "Environment.h"
#include "Value.h"

class Interpreter {
public:
    void execute(const std::vector<StmtPtr>& program);

private:
    Environment env;

    // ── Statement executors ───────────────────────────────────────────────────
    void executeStmt   (const Stmt& stmt);
    void executeDeclare(const DeclareStmt& s);
    void executeAssign (const AssignStmt& s);
    void executePrint  (const PrintStmt& s);

    // ── Expression evaluators ─────────────────────────────────────────────────
    Value evaluate      (const Expr& expr);
    Value evalLiteral   (const LiteralExpr& e);
    Value evalVar       (const VarExpr& e);
    Value evalBracket   (const BracketExpr& e);

    // ── Helpers ───────────────────────────────────────────────────────────────
    // Resolves [ ] bracket content (PDA-style strip/pass-through)
    std::string resolveEscapes(const std::string& input);
    Value       coerceLiteral (const std::string& raw);
};
