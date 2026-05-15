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
        Value val;
        if (initExpr) {
            val = evaluate(*initExpr);
            checkType(name, s.varType, val); // ← validate init value
        } else {
            switch (s.varType) {
                case TokenType::INT_TYPE:   val = Value{0};      break;
                case TokenType::FLOAT_TYPE: val = Value{0.0f};   break;
                case TokenType::BOOL_TYPE:  val = Value{false};  break;
                case TokenType::CHAR_TYPE:  val = Value{'\0'};   break;
                default:                    val = Value{std::string{""}}; break;
            }
        }
        env.declareVar(name, s.varType, val);
    }
}

void Interpreter::executeAssign(const AssignStmt& s) {
    if (!s.value) return;
    Value val = evaluate(*s.value);
    for (const auto& name : s.targets) {
        if (!env.has(name))
            throw std::runtime_error("Assignment to undeclared variable '" + name + "'");
        checkType(name, env.getType(name), val); // ← validate before setting
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
        if constexpr (std::is_same_v<T, BinaryExpr>)  return evalBinary(e);
        if constexpr (std::is_same_v<T, UnaryExpr>)   return evalUnary(e);
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

Value Interpreter::evalUnary(const UnaryExpr& e) {
    Value val = evaluate(*e.operand);
    if (e.op == "-") {
        if (std::holds_alternative<int>(val.data))
            return Value{-std::get<int>(val.data)};
        if (std::holds_alternative<float>(val.data))
            return Value{-std::get<float>(val.data)};
        throw std::runtime_error("Unary '-' requires a numeric operand");
    }
    if (e.op == "NOT") {
        if (std::holds_alternative<bool>(val.data))
            return Value{!std::get<bool>(val.data)};
        throw std::runtime_error("NOT requires a BOOL operand");
    }
    return val; // unary +
}

Value Interpreter::evalBinary(const BinaryExpr& e) {
    Value left  = evaluate(*e.left);
    Value right = evaluate(*e.right);

    // ── Arithmetic ──────────────────────────────────────────────────────────
    auto asFloat = [](const Value& v) -> float {
        if (std::holds_alternative<int>(v.data))   return (float)std::get<int>(v.data);
        if (std::holds_alternative<float>(v.data)) return std::get<float>(v.data);
        throw std::runtime_error("Expected numeric value");
    };
    auto isNumeric = [](const Value& v) {
        return std::holds_alternative<int>(v.data) || std::holds_alternative<float>(v.data);
    };
    auto bothInt = [](const Value& l, const Value& r) {
        return std::holds_alternative<int>(l.data) && std::holds_alternative<int>(r.data);
    };

    if (e.op == "+" || e.op == "-" || e.op == "*" || e.op == "/" || e.op == "%") {
        if (!isNumeric(left) || !isNumeric(right))
            throw std::runtime_error("Arithmetic operator '" + e.op + "' requires numeric operands");
        if (e.op == "/" && asFloat(right) == 0)
            throw std::runtime_error("Division by zero");
        if (e.op == "%" ) {
            if (!bothInt(left, right))
                throw std::runtime_error("Modulo requires INT operands");
            return Value{std::get<int>(left.data) % std::get<int>(right.data)};
        }
        if (bothInt(left, right)) {
            int l = std::get<int>(left.data), r = std::get<int>(right.data);
            if (e.op == "+") return Value{l + r};
            if (e.op == "-") return Value{l - r};
            if (e.op == "*") return Value{l * r};
            if (e.op == "/") return Value{l / r};
        }
        float l = asFloat(left), r = asFloat(right);
        if (e.op == "+") return Value{l + r};
        if (e.op == "-") return Value{l - r};
        if (e.op == "*") return Value{l * r};
        if (e.op == "/") return Value{l / r};
    }

    // ── Comparison ──────────────────────────────────────────────────────────
    if (e.op == "==" || e.op == "<>") {
        bool eq;
        if (isNumeric(left) && isNumeric(right))
            eq = asFloat(left) == asFloat(right);
        else
            eq = left.toString() == right.toString();
        return Value{e.op == "==" ? eq : !eq};
    }
    if (e.op == ">" || e.op == "<" || e.op == ">=" || e.op == "<=") {
        if (!isNumeric(left) || !isNumeric(right))
            throw std::runtime_error("Comparison '" + e.op + "' requires numeric operands");
        float l = asFloat(left), r = asFloat(right);
        if (e.op == ">")  return Value{l > r};
        if (e.op == "<")  return Value{l < r};
        if (e.op == ">=") return Value{l >= r};
        if (e.op == "<=") return Value{l <= r};
    }

    // ── Logical ─────────────────────────────────────────────────────────────
    if (e.op == "AND" || e.op == "OR") {
        if (!std::holds_alternative<bool>(left.data) || !std::holds_alternative<bool>(right.data))
            throw std::runtime_error(e.op + " requires BOOL operands");
        bool l = std::get<bool>(left.data), r = std::get<bool>(right.data);
        return Value{e.op == "AND" ? (l && r) : (l || r)};
    }

    throw std::runtime_error("Unknown operator '" + e.op + "'");
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

void Interpreter::checkType(const std::string& name, TokenType expected, const Value& val) {
    bool ok = false;
    std::string expectedName;

    switch (expected) {
        case TokenType::INT_TYPE:
            ok = std::holds_alternative<int>(val.data);
            expectedName = "INT";
            break;
        case TokenType::FLOAT_TYPE:
            ok = std::holds_alternative<float>(val.data);
            expectedName = "FLOAT";
            break;
        case TokenType::BOOL_TYPE:
            ok = std::holds_alternative<bool>(val.data);
            expectedName = "BOOL";
            break;
        case TokenType::CHAR_TYPE:
            ok = std::holds_alternative<char>(val.data);
            expectedName = "CHAR";
            break;
        default:
            return; // no check needed
    }

    if (!ok)
        throw std::runtime_error(
            "Type error: variable '" + name + "' is " + expectedName +
            " but got incompatible value '" + val.toString() + "'");
}