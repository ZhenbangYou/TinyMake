#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "exception.h"
#include "lexer.h"

namespace parser {

class ParserException : public RuntimeException {
   public:
    ParserException(const std::vector<std::string>& whatArgs)
        : RuntimeException(whatArgs) {}
};

enum ASTType { VAR_DEF, RULE };

struct AST {
    virtual ~AST() = default;
    virtual ASTType tokenType() const = 0;
    virtual std::string toString() const = 0;
};

struct VarDef final : public AST {
    lexer::Word varName;
    std::vector<std::variant<lexer::Word, lexer::Var>> values;

    VarDef(const lexer::Word& varName_,
           const std::vector<std::variant<lexer::Word, lexer::Var>>& values_)
        : varName(varName_), values(values_) {}
    ASTType tokenType() const override { return VAR_DEF; }
    std::string toString() const override {
        std::string result;
        result += "(Variable Assignment: ";
        result += "(Variable Name: " + varName.toString() + ")";
        result += "(Values:";
        for (const auto& v : values) {
            if (std::holds_alternative<lexer::Word>(v)) {
                result += '(' + std::get<lexer::Word>(v).toString() + ')';
            } else if (std::holds_alternative<lexer::Var>(v)) {
                result += '(' + std::get<lexer::Var>(v).toString() + ')';
            } else {
                throw ParserException(
                    {"Wrong type in VarDef, expect Word or Var"});
            }
        }
        result += ")";
        result += ")";
        return result;
    };
};

struct Rule final : public AST {
    std::vector<std::variant<lexer::Word, lexer::Var>> targets;
    std::vector<std::variant<lexer::Word, lexer::Var>> prereqs;
    std::vector<std::vector<
        std::variant<lexer::Word, lexer::Var, lexer::AutoVar, lexer::String>>>
        recipes;

    Rule(
        const std::vector<std::variant<lexer::Word, lexer::Var>>& targets_,
        const std::vector<std::variant<lexer::Word, lexer::Var>>& prereqs_,
        const std::vector<std::vector<std::variant<
            lexer::Word, lexer::Var, lexer::AutoVar, lexer::String>>>& recipes_)
        : targets(targets_), prereqs(prereqs_), recipes(recipes_) {}
    ASTType tokenType() const override { return RULE; }
    std::string toString() const override {
        std::string result;
        result += "(Rule:";

        result += "\t(Targets: ";
        for (const auto& t : targets) {
            if (std::holds_alternative<lexer::Word>(t)) {
                result += '(' + std::get<lexer::Word>(t).toString() + ')';
            } else if (std::holds_alternative<lexer::Var>(t)) {
                result += '(' + std::get<lexer::Var>(t).toString() + ')';
            }
        }
        result += ")\n";

        result += "\t(Prerequisites: ";
        for (const auto& p : prereqs) {
            if (std::holds_alternative<lexer::Word>(p)) {
                result += '(' + std::get<lexer::Word>(p).toString() + ')';
            } else if (std::holds_alternative<lexer::Var>(p)) {
                result += '(' + std::get<lexer::Var>(p).toString() + ')';
            }
        }
        result += ")\n";

        result += "\t(Recipes:\n";
        for (size_t i = 0; i < recipes.size(); i++) {
            result += "\t\t(Recipe " + std::to_string(i) + ": ";
            for (const auto& r : recipes[i]) {
                if (std::holds_alternative<lexer::Word>(r)) {
                    result += '(' + std::get<lexer::Word>(r).toString() + ')';
                } else if (std::holds_alternative<lexer::Var>(r)) {
                    result += '(' + std::get<lexer::Var>(r).toString() + ')';
                } else if (std::holds_alternative<lexer::AutoVar>(r)) {
                    result +=
                        '(' + std::get<lexer::AutoVar>(r).toString() + ')';
                } else if (std::holds_alternative<lexer::String>(r)) {
                    result += '(' + std::get<lexer::String>(r).toString() + ')';
                }
            }
            result += ")\n";
        }
        result += ")\n";

        result += ")";
        return result;
    };
};

std::pair<std::vector<VarDef>, std::vector<Rule>> parse(
    std::vector<std::shared_ptr<lexer::Token>> tokens);
}  // namespace parser