#include "parser.h"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "lexer.h"

namespace parser {
static std::optional<
    std::pair<VarDef, std::span<const std::shared_ptr<lexer::Token>>>>
parseVarDef(std::span<const std::shared_ptr<lexer::Token>> tokenStream) {
    if (tokenStream.size() >= 2 && tokenStream[0]->tokenType() == lexer::WORD &&
        tokenStream[1]->tokenType() == lexer::EQUAL) {
        lexer::Word varName =
            *std::dynamic_pointer_cast<lexer::Word>(tokenStream[0]);
        tokenStream = tokenStream.subspan(2);

        std::vector<std::variant<lexer::Word, lexer::Var>> values;
        while (!tokenStream.empty() &&
               tokenStream.front()->tokenType() != lexer::ENDL) {
            std::shared_ptr<lexer::Token> t = tokenStream.front();
            if (t->tokenType() == lexer::WORD) {
                values.emplace_back(*std::dynamic_pointer_cast<lexer::Word>(t));
            } else if (t->tokenType() == lexer::VAR) {
                values.emplace_back(*std::dynamic_pointer_cast<lexer::Var>(t));
            } else {
                return {};
            }
            tokenStream = tokenStream.subspan(1);
        }
        return {{VarDef(varName, values), tokenStream}};
    }
    return {};
}

static std::optional<
    std::pair<Rule, std::span<const std::shared_ptr<lexer::Token>>>>
parseRule(std::span<const std::shared_ptr<lexer::Token>> tokenStream) {
    if (!(!tokenStream.empty() &&
          (tokenStream.front()->tokenType() == lexer::WORD ||
           tokenStream.front()->tokenType() == lexer::VAR))) {
        return {};
    }

    std::vector<std::variant<lexer::Word, lexer::Var>> targets;
    while (!tokenStream.empty() &&
           tokenStream.front()->tokenType() != lexer::COLON) {
        auto t = tokenStream.front();
        if (t->tokenType() == lexer::WORD) {
            targets.emplace_back(
                *std::dynamic_pointer_cast<lexer::Word>(tokenStream.front()));
            tokenStream = tokenStream.subspan(1);
        } else if (t->tokenType() == lexer::VAR) {
            targets.emplace_back(
                *std::dynamic_pointer_cast<lexer::Var>(tokenStream.front()));
            tokenStream = tokenStream.subspan(1);
        } else {
            return {};
        }
    }

    if (!(!tokenStream.empty() &&
          tokenStream.front()->tokenType() == lexer::COLON)) {
        return {};
    }
    tokenStream = tokenStream.subspan(1);

    std::vector<std::variant<lexer::Word, lexer::Var>> prereqs;
    while (!tokenStream.empty() &&
           tokenStream.front()->tokenType() != lexer::ENDL) {
        auto t = tokenStream.front();
        if (t->tokenType() == lexer::WORD) {
            prereqs.emplace_back(
                *std::dynamic_pointer_cast<lexer::Word>(tokenStream.front()));
            tokenStream = tokenStream.subspan(1);
        } else if (t->tokenType() == lexer::VAR) {
            prereqs.emplace_back(
                *std::dynamic_pointer_cast<lexer::Var>(tokenStream.front()));
            tokenStream = tokenStream.subspan(1);
        }
    }

    if (!tokenStream.empty() &&
        tokenStream.front()->tokenType() != lexer::ENDL) {
        return {};
    }

    std::vector<std::vector<
        std::variant<lexer::Word, lexer::Var, lexer::AutoVar, lexer::String>>>
        recipes;
    while (true) {
        while (!tokenStream.empty() &&
               tokenStream.front()->tokenType() == lexer::ENDL) {
            tokenStream = tokenStream.subspan(1);
        }
        if (tokenStream.empty()) {
            break;
        }

        // Parse exactly one line

        if (tokenStream.front()->tokenType() != lexer::TAB) {
            break;
        }
        while (!tokenStream.empty() &&
               tokenStream.front()->tokenType() == lexer::TAB) {
            tokenStream = tokenStream.subspan(1);
        }

        std::vector<std::variant<lexer::Word, lexer::Var, lexer::AutoVar,
                                 lexer::String>>
            recipe;
        while (!tokenStream.empty() &&
               tokenStream.front()->tokenType() != lexer::ENDL) {
            std::shared_ptr<lexer::Token> t = tokenStream.front();
            if (t->tokenType() == lexer::WORD) {
                recipe.emplace_back(*std::dynamic_pointer_cast<lexer::Word>(
                    tokenStream.front()));
                tokenStream = tokenStream.subspan(1);
            } else if (t->tokenType() == lexer::VAR) {
                recipe.emplace_back(*std::dynamic_pointer_cast<lexer::Var>(
                    tokenStream.front()));
                tokenStream = tokenStream.subspan(1);
            } else if (t->tokenType() == lexer::AUTO_VAR) {
                recipe.emplace_back(*std::dynamic_pointer_cast<lexer::AutoVar>(
                    tokenStream.front()));
                tokenStream = tokenStream.subspan(1);
            } else if (t->tokenType() == lexer::STRING) {
                recipe.emplace_back(*std::dynamic_pointer_cast<lexer::String>(
                    tokenStream.front()));
                tokenStream = tokenStream.subspan(1);
            } else {
                return {};
            }
        }
        if (!recipe.empty()) {
            recipes.emplace_back(recipe);
        }
    }
    return {{Rule(targets, prereqs, recipes), tokenStream}};
}

std::pair<std::vector<VarDef>, std::vector<Rule>> parse(
    std::vector<std::shared_ptr<lexer::Token>> tokens) {
    std::span<const std::shared_ptr<lexer::Token>> tokenStream(tokens);
    std::vector<VarDef> varDefs;
    std::vector<Rule> rules;
    while (true) {
        while (!tokenStream.empty() &&
               tokenStream.front()->tokenType() == lexer::ENDL) {
            tokenStream = tokenStream.subspan(1);
        }
        if (tokenStream.empty()) {
            break;
        }

        auto tryVarDefRes = parseVarDef(tokenStream);
        if (tryVarDefRes.has_value()) {
            auto [varDef, nextTokenStream] = tryVarDefRes.value();
            varDefs.emplace_back(std::move(varDef));
            tokenStream = nextTokenStream;
            continue;
        }

        auto tryRuleRes = parseRule(tokenStream);
        if (tryRuleRes.has_value()) {
            auto [rule, nextTokenStream] = tryRuleRes.value();
            rules.emplace_back(std::move(rule));
            tokenStream = nextTokenStream;
            continue;
        }
        break;
    }
    if (tokenStream.empty()) {
        return {varDefs, rules};
    } else {
        throw ParserException(
            {"Parse fail at line:", std::to_string(tokenStream.front()->lineno),
             ", next token", tokenStream.front()->toString()});
    }
}
}  // namespace parser