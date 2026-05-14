#pragma once
#include <variant>
#include <string>
#include <stdexcept>

// Holds any runtime value the language supports
struct Value {
    using Data = std::variant<int, float, char, bool, std::string>;
    Data data;

    // ── Factories ────────────────────────────────────────────────────────────
    static Value fromInt   (int v)         { return {v}; }
    static Value fromFloat (float v)       { return {v}; }
    static Value fromChar  (char v)        { return {v}; }
    static Value fromBool  (bool v)        { return {v}; }
    static Value fromString(std::string v) { return {std::move(v)}; }

    // ── Stringify (used by PRINT) ─────────────────────────────────────────────
    std::string toString() const {
        return std::visit([](auto&& v) -> std::string {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, int>)         return std::to_string(v);
            if constexpr (std::is_same_v<T, float>)       return std::to_string(v);
            if constexpr (std::is_same_v<T, char>)        return std::string(1, v);
            if constexpr (std::is_same_v<T, bool>)        return v ? "TRUE" : "FALSE";
            if constexpr (std::is_same_v<T, std::string>) return v;
        }, data);
    }
};
