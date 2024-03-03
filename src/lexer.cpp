#include "lexer.h"

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

namespace lexer {

static bool isInCharSet(char c) {
    std::unordered_set<char> specialChars{'_', '.', '%', '/',
                                          '-', ',', '@', '\''};
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') || specialChars.contains(c);
}

static std::optional<
    std::tuple<std::shared_ptr<Token>, std::string_view, size_t>>
lexWord(std::string_view charStream, size_t lineno) {
    std::string name;
    while (!charStream.empty()) {
        char c = charStream.front();
        if (isInCharSet(c)) {
            name += c;
            charStream = charStream.substr(1);
        } else {
            break;
        }
    }
    if (name.empty()) {
        return {};
    }
    return {{std::make_shared<Word>(name, lineno), charStream, lineno}};
}

static std::optional<
    std::tuple<std::shared_ptr<Token>, std::string_view, size_t>>
lexVar(std::string_view charStream, size_t lineno) {
    if (charStream.starts_with("$(")) {
        charStream = charStream.substr(2);
        while (!charStream.empty() && charStream.front() == ' ') {
            charStream = charStream.substr(1);
        }
        auto res = lexWord(charStream, lineno);
        if (res.has_value()) {
            auto [word, nextInputView, nextLineno] = res.value();
            charStream = nextInputView;
            lineno = nextLineno;
            while (!charStream.empty() && charStream.front() == ' ') {
                charStream = charStream.substr(1);
            }
            if (!charStream.empty() && charStream.front() == ')') {
                return {{std::make_shared<Var>(
                             dynamic_pointer_cast<Word>(word)->name, lineno),
                         charStream.substr(1), lineno}};
            } else {
                return {};
            }
        } else {
            return {};
        }
    } else if (charStream.starts_with('$')) {
        charStream = charStream.substr(1);
        if (charStream.empty()) {
            return {{std::make_shared<Word>("$", lineno), charStream.substr(1),
                     lineno}};
        }
        char c = charStream.front();
        if (isInCharSet(c)) {
            return {{std::make_shared<Var>(std::string(1, c), lineno),
                     charStream.substr(1), lineno}};
        } else if (c == ' ') {
            return {{std::make_shared<Word>("", lineno), charStream.substr(1),
                     lineno}};
        } else if (c == '$' || c == '\n') {
            return {{std::make_shared<Word>("$", lineno), charStream.substr(1),
                     lineno}};
        } else {
            return {};
        }
    } else {
        return {};
    }
}

static std::optional<
    std::tuple<std::shared_ptr<Token>, std::string_view, size_t>>
lexAutoVar(std::string_view charStream, size_t lineno) {
    if (charStream.starts_with("$@")) {
        return {{std::make_shared<AutoVar>(AutoVar::DOLLAR_AT, lineno),
                 charStream.substr(2), lineno}};
    } else if (charStream.starts_with("$<")) {
        return {{std::make_shared<AutoVar>(AutoVar::DOLLAR_LT, lineno),
                 charStream.substr(2), lineno}};
    } else if (charStream.starts_with("$^")) {
        return {{std::make_shared<AutoVar>(AutoVar::DOLLAR_SUP, lineno),
                 charStream.substr(2), lineno}};
    } else {
        return {};
    }
}

static std::optional<
    std::tuple<std::shared_ptr<Token>, std::string_view, size_t>>
lexString(std::string_view charStream, size_t lineno) {
    if (charStream.starts_with('"')) {
        charStream = charStream.substr(1);
        std::vector<std::variant<std::string, Var, AutoVar>> segments;
        std::string curSeg;
        while (!charStream.empty()) {
            char c = charStream.front();
            if (c == '\n') {
                return {};
            } else if (c == '"') {
                if (!curSeg.empty()) {
                    segments.emplace_back(curSeg);
                }
                charStream = charStream.substr(1);
                break;
            } else if (c == '$') {
                if (!curSeg.empty()) {
                    segments.emplace_back(curSeg);
                    curSeg.clear();
                }

                auto tryVarRes = lexVar(charStream, lineno);
                if (tryVarRes.has_value()) {
                    auto [var, nextInputView, nextLineno] = tryVarRes.value();
                    segments.emplace_back(*std::dynamic_pointer_cast<Var>(var));
                    charStream = nextInputView;
                    lineno = nextLineno;
                } else {
                    auto tryAutoVarRes = lexAutoVar(charStream, lineno);
                    if (tryAutoVarRes.has_value()) {
                        auto [autoVar, nextInputView, nextLineno] =
                            tryAutoVarRes.value();
                        segments.emplace_back(
                            *std::dynamic_pointer_cast<Var>(autoVar));
                        charStream = nextInputView;
                        lineno = nextLineno;
                    } else {
                        return {};
                    }
                }
            } else if (c == '\\') {
                charStream = charStream.substr(1);
                if (charStream.empty()) {
                    return {};
                }
                std::unordered_map<char, char> escapeChars{
                    {'"', '\"'}, {'\'', '\''}, {'\\', '\\'}, {'#', '#'},
                    {'n', '\n'}, {'r', '\r'},  {'t', '\t'},
                };
                if (escapeChars.contains(charStream.front())) {
                    curSeg += escapeChars[charStream.front()];
                } else {
                    return {};
                }
                charStream = charStream.substr(1);
            } else {
                curSeg += c;
                charStream = charStream.substr(1);
            }
        }
        return {
            {std::make_shared<String>(segments, lineno), charStream, lineno}};
    } else {
        return {};
    }
}

static std::optional<
    std::tuple<std::shared_ptr<Token>, std::string_view, size_t>>
lexEqual(std::string_view charStream, size_t lineno) {
    if (charStream.starts_with('=')) {
        return {
            {std::make_shared<Equal>(lineno), charStream.substr(1), lineno}};
    } else if (charStream.starts_with(":=")) {
        return {
            {std::make_shared<Equal>(lineno), charStream.substr(2), lineno}};
    } else {
        return {};
    }
}

static std::optional<
    std::tuple<std::shared_ptr<Token>, std::string_view, size_t>>
lexColon(std::string_view charStream, size_t lineno) {
    if (charStream.starts_with(':')) {
        return {
            {std::make_shared<Colon>(lineno), charStream.substr(1), lineno}};
    } else {
        return {};
    }
}

static std::optional<
    std::tuple<std::shared_ptr<Token>, std::string_view, size_t>>
lexTab(std::string_view charStream, size_t lineno) {
    if (charStream.starts_with('\t')) {
        return {{std::make_shared<Tab>(lineno), charStream.substr(1), lineno}};
    } else {
        return {};
    }
}

static std::optional<
    std::tuple<std::shared_ptr<Token>, std::string_view, size_t>>
lexEndl(std::string_view charStream, size_t lineno) {
    if (charStream.starts_with('\n')) {
        return {
            {std::make_shared<Endl>(lineno), charStream.substr(1), lineno + 1}};
    }
    return {};
}

static std::string_view lexIgnore(std::string_view charStream) {
    if (charStream.starts_with(' ')) {
        while (!charStream.empty() && charStream.starts_with(" ")) {
            charStream = charStream.substr(1);
        }
    }
    if (charStream.starts_with('#')) {
        while (!charStream.empty() && !charStream.starts_with("\n")) {
            charStream = charStream.substr(1);
        }
    }
    return charStream;
}

std::vector<std::shared_ptr<Token>> lex(const std::string& sourceCode) {
    std::vector lexers{lexWord,  lexVar,   lexAutoVar, lexString,
                       lexEqual, lexColon, lexTab,     lexEndl};
    std::string_view charStream(sourceCode);
    std::vector<std::shared_ptr<Token>> tokenStream;
    size_t lineno = 1;
    while (true) {
        charStream = lexIgnore(charStream);
        if (charStream.empty()) {
            break;
        }
        if (charStream.starts_with("\\\n")) {
            lineno++;
            charStream = charStream.substr(2);
        } else {
            bool successful = false;
            for (auto lexer : lexers) {
                auto res = lexer(charStream, lineno);
                if (res.has_value()) {
                    auto [token, nextInputView, nextLineno] = res.value();
                    tokenStream.emplace_back(token);
                    charStream = nextInputView;
                    lineno = nextLineno;
                    successful = true;
                    break;
                }
            }
            if (!successful) {
                throw LexerException({"Lexing failed at line",
                                      std::to_string(lineno),
                                      "unrecognized token:",
                                      std::string(1, charStream.front())});
            }
        }
    }
    return tokenStream;
}
}  // namespace lexer
