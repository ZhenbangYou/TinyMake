#pragma once

#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "lexer.h"
#include "var-replacement.h"

namespace auto_var_replacement {

struct Rule final {
    std::vector<std::string> targets;
    std::vector<std::string> prereqs;
    std::vector<std::string> recipes;
    size_t lineno;

    Rule(const std::vector<std::string>& targets_,
         const std::vector<std::string>& prereqs_,
         const std::vector<std::string>& recipes_, size_t lineno_)
        : targets(targets_),
          prereqs(prereqs_),
          recipes(recipes_),
          lineno(lineno_) {}
    std::string toString() const {
        std::string result;
        result += "(Rule:";

        result += ")";
        return result;
    };
};

std::shared_ptr<Rule> replace(
    const std::shared_ptr<var_replacement::Rule>& rule);
}  // namespace auto_var_replacement