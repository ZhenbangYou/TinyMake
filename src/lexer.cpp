#include "lexer.h"

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace lexer {

static bool isInCharSet(char c) {
    std::unordered_set<char> specialChars{'_', '.', '%', '/',
                                          '-', ',', '@', '\''};
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') || specialChars.contains(c);
}

static std::optional<std::pair<std::shared_ptr<Token>, std::string_view>>
lexWord(std::string_view inputView) {
    std::string name;
    while (!inputView.empty()) {
        char c = inputView.front();
        if (isInCharSet(c)) {
            name += c;
            inputView = inputView.substr(1);
        } else {
            break;
        }
    }
    if (name.empty()) {
        return {};
    }
    return {{std::make_shared<Word>(name), inputView}};
}

static std::optional<std::pair<std::shared_ptr<Token>, std::string_view>>
lexVar(std::string_view inputView) {
    if (inputView.starts_with("$(")) {
        inputView = inputView.substr(2);
        while (!inputView.empty() && inputView.front() == ' ') {
            inputView = inputView.substr(1);
        }
        auto res = lexWord(inputView);
        if (res.has_value()) {
            auto [word, nextInputView] = res.value();
            inputView = nextInputView;
            while (!inputView.empty() && inputView.front() == ' ') {
                inputView = inputView.substr(1);
            }
            if (!inputView.empty() && inputView.front() == ')') {
                return {{std::make_shared<Var>(
                             dynamic_pointer_cast<Word>(word)->name),
                         inputView.substr(1)}};
            } else {
                return {};
            }
        } else {
            return {};
        }
    } else if (inputView.starts_with('$')) {
        inputView = inputView.substr(1);
        char c = inputView.front();
        if (isInCharSet(c)) {
            return {{std::make_shared<Var>(std::string(1, c)),
                     inputView.substr(1)}};
        } else if (c == ' ') {
            return {{std::make_shared<Word>(""), inputView.substr(1)}};
        } else if (c == '$') {
            return {{std::make_shared<Word>("$"), inputView.substr(1)}};
        } else {
            return {};
        }
    } else {
        return {};
    }
}

static std::optional<std::pair<std::shared_ptr<Token>, std::string_view>>
lexAutoVar(std::string_view inputView) {
    if (inputView.starts_with("$@")) {
        return {{std::make_shared<AutoVar>(AutoVar::DOLLAR_AT),
                 inputView.substr(2)}};
    } else if (inputView.starts_with("$<")) {
        return {{std::make_shared<AutoVar>(AutoVar::DOLLAR_LT),
                 inputView.substr(2)}};
    } else if (inputView.starts_with("$^")) {
        return {{std::make_shared<AutoVar>(AutoVar::DOLLAR_SUP),
                 inputView.substr(2)}};
    } else {
        return {};
    }
}

static std::optional<std::pair<std::shared_ptr<Token>, std::string_view>>
lexString(std::string_view inputView) {
    if (inputView.starts_with('"')) {
        inputView = inputView.substr(1);
        std::vector<std::variant<std::string, Var, AutoVar>> segments;
        std::string curSeg;
        while (!inputView.empty()) {
            char c = inputView.front();
            if (c == '\n') {
                return {};
            } else if (c == '"') {
                if (!curSeg.empty()) {
                    segments.emplace_back(curSeg);
                }
                inputView = inputView.substr(1);
                break;
            } else if (c == '$') {
                if (!curSeg.empty()) {
                    segments.emplace_back(curSeg);
                    curSeg.clear();
                }

                auto tryVarRes = lexVar(inputView);
                if (tryVarRes.has_value()) {
                    auto [var, nextInputView] = tryVarRes.value();
                    segments.emplace_back(*std::dynamic_pointer_cast<Var>(var));
                    inputView = nextInputView;
                } else {
                    auto tryAutoVarRes = lexAutoVar(inputView);
                    if (tryAutoVarRes.has_value()) {
                        auto [autoVar, nextInputView] = tryAutoVarRes.value();
                        segments.emplace_back(
                            *std::dynamic_pointer_cast<Var>(autoVar));
                        inputView = nextInputView;
                    } else {
                        return {};
                    }
                }
            } else if (c == '\\') {
                inputView = inputView.substr(1);
                if (inputView.empty()) {
                    return {};
                }
                std::unordered_map<char, char> escapeChars{
                    {'"', '\"'}, {'\'', '\''}, {'\\', '\\'}, {'#', '#'},
                    {'n', '\n'}, {'r', '\r'},  {'t', '\t'},
                };
                if (escapeChars.contains(inputView.front())) {
                    curSeg += escapeChars[inputView.front()];
                } else {
                    return {};
                }
                inputView = inputView.substr(1);
            } else {
                curSeg += c;
                inputView = inputView.substr(1);
            }
        }
        return {{std::make_shared<String>(segments), inputView}};
    } else {
        return {};
    }
}

static std::optional<std::pair<std::shared_ptr<Token>, std::string_view>>
lexEqual(std::string_view inputView) {
    if (inputView.starts_with('=')) {
        return {{std::make_shared<Equal>(), inputView.substr(1)}};
    } else {
        return {};
    }
}

static std::optional<std::pair<std::shared_ptr<Token>, std::string_view>>
lexColon(std::string_view inputView) {
    if (inputView.starts_with(':')) {
        return {{std::make_shared<Colon>(), inputView.substr(1)}};
    } else {
        return {};
    }
}

static std::optional<std::pair<std::shared_ptr<Token>, std::string_view>>
lexTab(std::string_view inputView) {
    if (inputView.starts_with('\t')) {
        return {{std::make_shared<Tab>(), inputView.substr(1)}};
    } else {
        return {};
    }
}

static std::string_view lexIgnore(std::string_view inputView) {
    if (inputView.starts_with(' ')) {
        while (!inputView.empty() && inputView.starts_with(" ")) {
            inputView = inputView.substr(1);
        }
    }
    if (inputView.starts_with('#')) {
        while (!inputView.empty() && !inputView.starts_with("\n")) {
            inputView = inputView.substr(1);
        }
    }
    return inputView;
}

std::vector<std::shared_ptr<Token>> lex(const std::string& input) {
    std::vector lexers{lexWord,  lexVar,   lexAutoVar, lexString,
                       lexEqual, lexColon, lexTab};
    std::string_view inputView(input);
    std::vector<std::shared_ptr<Token>> tokenStream;
    size_t lineno = 1;
    while (true) {
        inputView = lexIgnore(inputView);
        if (inputView.empty()) {
            break;
        }
        if (inputView.starts_with('\n')) {
            tokenStream.emplace_back(std::make_shared<Endl>());
            lineno++;
            inputView = inputView.substr(1);
        } else if (inputView.starts_with("\r\n")) {
            tokenStream.emplace_back(std::make_shared<Endl>());
            lineno++;
            inputView = inputView.substr(2);
        } else if (inputView.starts_with("\\\n")) {
            lineno++;
            inputView = inputView.substr(2);
        } else {
            bool successful = false;
            for (auto lexer : lexers) {
                auto res = lexer(inputView);
                if (res.has_value()) {
                    auto [token, nextInputView] = res.value();
                    tokenStream.emplace_back(token);
                    inputView = nextInputView;
                    successful = true;
                    break;
                }
            }
            if (!successful) {
                throw LexerException(
                    {"Lexing failed at line", std::to_string(lineno),
                     "unrecognized token:", std::string(1, inputView.front())});
            }
        }
    }
    return tokenStream;
}
}  // namespace lexer
