#pragma once
#include <map>
#include <string>
#include <optional>
#include "Value.h"

class Environment {
public:
    void set(const std::string& name, Value val) {
        store[name] = std::move(val);
    }

    // Returns the stored Value or an empty string Value if not found
    Value get(const std::string& name) const {
        auto it = store.find(name);
        if (it == store.end()) return Value{std::string{""}};
        return it->second;
    }

    bool has(const std::string& name) const {
        return store.count(name) > 0;
    }

private:
    std::map<std::string, Value> store;
};
