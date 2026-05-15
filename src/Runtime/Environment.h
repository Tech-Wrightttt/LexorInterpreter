#pragma once
#include <string>
#include <unordered_map>
#include "../Lexer/Token.h"
#include "Value.h"

class Environment {
public:
    void declareVar(const std::string& name, TokenType type, Value val) {
        types[name] = type;
        store[name] = val;
    }

    void set(const std::string& name, Value val) {
        store[name] = val;
    }

    Value get(const std::string& name) const {
        auto it = store.find(name);
        if (it == store.end())
            throw std::runtime_error("Undefined variable '" + name + "'");
        return it->second;
    }

    bool has(const std::string& name) const {
        return store.count(name) > 0;
    }

    TokenType getType(const std::string& name) const {
        auto it = types.find(name);
        if (it == types.end())
            throw std::runtime_error("No type info for variable '" + name + "'");
        return it->second;
    }

private:
    std::unordered_map<std::string, Value>     store;
    std::unordered_map<std::string, TokenType> types;
};