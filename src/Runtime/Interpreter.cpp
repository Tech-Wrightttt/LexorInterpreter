#include "Interpreter.h"
#include <iostream>
#include <stdexcept>
#include <sstream>

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
        else if constexpr (std::is_same_v<T, ScanStmt>) executeScan(s);
        else if constexpr (std::is_same_v<T, IfStmt>)
            throw std::runtime_error("IfStmt not yet implemented");
    }, stmt.data);
}

void Interpreter::executeDeclare(const DeclareStmt& s) {
    for (const auto& [name, initExpr] : s.vars) {
        if (initExpr) {
            env.set(name, evaluate(*initExpr));
        } else {
            // Default value depends on declared type
            switch (s.varType) {
                case TokenType::INT_TYPE:   env.set(name, Value{0});      break;
                case TokenType::FLOAT_TYPE: env.set(name, Value{0.0f});   break;
                case TokenType::BOOL_TYPE:  env.set(name, Value{false});  break;
                case TokenType::CHAR_TYPE:  env.set(name, Value{'\0'});   break;
                default:                    env.set(name, Value{std::string{""}}); break;
            }
        }
    }
}

void Interpreter::executeAssign(const AssignStmt& s) {
    if (!s.value) return;
    Value val = evaluate(*s.value);
    for (const auto& name : s.targets) {
        if (!env.has(name))
            throw std::runtime_error("Assignment to undeclared variable '" + name + "'");
        env.set(name, val);
    }
}

void Interpreter::executePrint(const PrintStmt& s) {
    for (const auto& part : s.parts)
        std::cout << evaluate(*part).toString();
}

void Interpreter::executeScan(const ScanStmt& s) {
    // Spec: multiple values entered separated by comma
    std::string line;
    std::getline(std::cin, line);
    std::stringstream ss(line);
    std::string token;
    size_t i = 0;

    while (std::getline(ss, token, ',') && i < s.targets.size()) {
        // Trim whitespace
        token.erase(0, token.find_first_not_of(" \t"));
        token.erase(token.find_last_not_of(" \t") + 1);

        const std::string& name = s.targets[i++];
        if (!env.has(name))
            throw std::runtime_error("SCAN target '" + name + "' is not declared");
        env.set(name, coerceLiteral(token));
    }
    if (i < s.targets.size())
        throw std::runtime_error("SCAN expected " + std::to_string(s.targets.size()) +
                                 " value(s) but got " + std::to_string(i));
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
