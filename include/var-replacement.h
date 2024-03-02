#pragma once

#include <string>
#include <variant>
#include <vector>

#include "lexer.h"
#include "parser.h"

namespace var_replacement {

struct String final {
    std::vector<std::variant<std::string, lexer::AutoVar>> segments;

    explicit String(
        const std::vector<std::variant<std::string, lexer::AutoVar>>& segments_)
        : segments(segments_) {}
    std::string toString() const {
        std::string result("(String ");
        for (const auto& seg : segments) {
            if (seg.index() == 0) {
                result += std::get<std::string>(seg);
            } else {
                result += std::get<lexer::AutoVar>(seg).toString();
            }
        }
        result += ')';
        return result;
    }
};

struct Rule final {
    std::vector<std::string> targets;
    std::vector<std::string> prereqs;
    std::vector<std::variant<std::string, lexer::AutoVar, String>> recipes;
    size_t lineno;

    Rule(const std::vector<std::string>& targets_,
         const std::vector<std::string>& prereqs_,
         const std::vector<std::variant<std::string, lexer::AutoVar, String>>&
             recipes_,
         size_t lineno_)
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

std::shared_ptr<Rule> replace(const std::shared_ptr<parser::Rule>& rule);
}  // namespace var_replacement