#include "Interpreter.h"
#include <iostream>
#include <stdexcept>

// ── Public ────────────────────────────────────────────────────────────────────

void Interpreter::execute(const std::vector<StmtPtr>& program) {
    for (const auto& stmt : program)
        executeStmt(*stmt);
}

// ── Statement dispatch ────────────────────────────────────────────────────────

void Interpreter::executeStmt(const Stmt& stmt) {
    std::visit([this](const auto& s) {
        using T = std::decay_t<decltype(s)>;
        if constexpr (std::is_same_v<T, DeclareStmt>) executeDeclare(s);
        else if constexpr (std::is_same_v<T, AssignStmt>)  executeAssign(s);
        else if constexpr (std::is_same_v<T, PrintStmt>)   executePrint(s);
        else if constexpr (std::is_same_v<T, IfStmt>)
            throw std::runtime_error("IfStmt not yet implemented");
    }, stmt.data);
}

void Interpreter::executeDeclare(const DeclareStmt& s) {
    for (const auto& [name, initExpr] : s.vars) {
        if (initExpr) env.set(name, evaluate(*initExpr));
        else          env.set(name, Value{std::string{""}});
    }
}

void Interpreter::executeAssign(const AssignStmt& s) {
    if (!s.value) return;
    Value val = evaluate(*s.value);
    for (const auto& name : s.targets)
        env.set(name, val);
}

void Interpreter::executePrint(const PrintStmt& s) {
    for (const auto& part : s.parts)
        std::cout << evaluate(*part).toString();
}

// ── Expression evaluation ─────────────────────────────────────────────────────

Value Interpreter::evaluate(const Expr& expr) {
    return std::visit([this](const auto& e) -> Value {
        using T = std::decay_t<decltype(e)>;
        if constexpr (std::is_same_v<T, LiteralExpr>) return evalLiteral(e);
        if constexpr (std::is_same_v<T, VarExpr>)     return evalVar(e);
        if constexpr (std::is_same_v<T, BracketExpr>) return evalBracket(e);
        if constexpr (std::is_same_v<T, BinaryExpr>)
            throw std::runtime_error("BinaryExpr not yet implemented");
        return Value{std::string{""}};
    }, expr.data);
}

Value Interpreter::evalLiteral(const LiteralExpr& e) {
    return coerceLiteral(e.value);
}

Value Interpreter::evalVar(const VarExpr& e) {
    return env.get(e.name);
}

Value Interpreter::evalBracket(const BracketExpr& e) {
    // Concatenate all inner parts into a single string value
    std::string result;
    for (const auto& part : e.parts)
        result += evaluate(*part).toString();
    return Value{result};
}

// ── Helpers ───────────────────────────────────────────────────────────────────

// PDA-style bracket stripping — same logic as original resolveEscapes
std::string Interpreter::resolveEscapes(const std::string& input) {
    std::string result;
    size_t i = 0;
    while (i < input.size()) {
        if (input[i] == '[') {
            size_t close = i + 1;
            while (close < input.size() && input[close] != ']') close++;
            if (close < input.size()) {
                result += input.substr(i + 1, close - i - 1);
                i = close + 1;
            } else {
                result += input[i++];
            }
        } else {
            result += input[i++];
        }
    }
    return result;
}

// Attempt to parse a literal string into the most specific Value type
Value Interpreter::coerceLiteral(const std::string& raw) {
    if (raw == "TRUE")  return Value{true};
    if (raw == "FALSE") return Value{false};

    // Try int
    try {
        size_t idx;
        int iv = std::stoi(raw, &idx);
        if (idx == raw.size()) return Value{iv};
    } catch (...) {}

    // Try float
    try {
        size_t idx;
        float fv = std::stof(raw, &idx);
        if (idx == raw.size()) return Value{fv};
    } catch (...) {}

    // Single char
    if (raw.size() == 1) return Value{raw[0]};

    return Value{raw};
}
