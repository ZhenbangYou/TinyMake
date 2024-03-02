#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "exception.h"

namespace lexer {

class LexerException : public RuntimeException {
   public:
    LexerException(const std::vector<std::string>& whatArgs)
        : RuntimeException(whatArgs) {}
};
enum TokenType { WORD, VAR, AUTO_VAR, STRING, EQUAL, COLON, TAB, ENDL };

struct Token {
    virtual ~Token() = default;
    virtual TokenType tokenType() const = 0;
    virtual std::string toString() const = 0;
};

struct Word final : public Token {
    std::string name;

    explicit Word(const std::string& name_) : name(name_) {}
    TokenType tokenType() const override { return WORD; }
    std::string toString() const override { return "(Word " + name + ")"; }
};

struct Var final : public Token {
    std::string name;

    explicit Var(const std::string& name_) : name(name_) {}
    TokenType tokenType() const override { return VAR; }
    std::string toString() const override { return "(Var " + name + ")"; }
};

struct AutoVar final : public Token {
    enum Type { DOLLAR_AT, DOLLAR_LT, DOLLAR_SUP } type;
    static std::string typeToString(Type type) {
        switch (type) {
            case DOLLAR_AT:
                return "$@";
            case DOLLAR_LT:
                return "$<";
            case DOLLAR_SUP:
                return "$^";
        }
        throw LexerException({"unreachable"});
    }

    explicit AutoVar(Type type_) : type(type_) {}
    TokenType tokenType() const override { return VAR; }
    std::string toString() const override {
        return "(AutoVar " + typeToString(type) + ")";
    }
};

struct String final : public Token {
    std::vector<std::variant<std::string, Var, AutoVar>> segments;

    explicit String(
        const std::vector<std::variant<std::string, Var, AutoVar>>& segments_)
        : segments(segments_) {}
    TokenType tokenType() const override { return STRING; }
    std::string toString() const override {
        std::string result("(String ");
        for (const auto& seg : segments) {
            if (seg.index() == 0) {
                result += std::get<std::string>(seg);
            } else if (seg.index() == 1) {
                result += std::get<Var>(seg).toString();
            } else {
                result += std::get<AutoVar>(seg).toString();
            }
        }
        result += ')';
        return result;
    }
};

struct Equal final : public Token {
    TokenType tokenType() const override { return EQUAL; }
    std::string toString() const override { return "(Equal)"; }
};

struct Colon final : public Token {
    TokenType tokenType() const override { return COLON; }
    std::string toString() const override { return "(Colon)"; }
};

struct Tab final : public Token {
    TokenType tokenType() const override { return TAB; }
    std::string toString() const override { return "(Tab)"; }
};

struct Endl final : public Token {
    TokenType tokenType() const override { return ENDL; }
    std::string toString() const override { return "(Endl)"; }
};

std::vector<std::shared_ptr<Token>> lex(const std::string& input);
}  // namespace lexer
