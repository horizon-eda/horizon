#pragma once
#include "set.hpp"
#include <vector>
#include <functional>
#include <memory>
#include <string>
#include <algorithm>

namespace horizon {
// using json = nlohmann::json;

class ParameterProgram {
    friend class ParameterCommands;

public:
    ParameterProgram(const std::string &s);
    ParameterProgram(const ParameterProgram &other);
    ParameterProgram &operator=(const ParameterProgram &other);
    std::pair<bool, std::string> get_init_error();
    const std::string &get_code() const;
    std::pair<bool, std::string> set_code(const std::string &s);

    std::pair<bool, std::string> run(const ParameterSet &pset = {});
    using Stack = std::vector<int64_t>;
    const auto &get_stack() const
    {
        return stack;
    }

    bool stack_pop(int64_t &va);

protected:
    class Token {
    public:
        enum class Type { INT, CMD, STR, UUID };
        Token(Type ty) : type(ty)
        {
        }

        const Type type;

        virtual ~Token()
        {
        }

        virtual std::unique_ptr<Token> clone() const = 0;
    };

    class TokenInt : public Token {
    public:
        TokenInt(int64_t v) : Token(Token::Type::INT), value(v)
        {
        }

        const int64_t value;

        std::unique_ptr<Token> clone() const override
        {
            return std::make_unique<TokenInt>(*this);
        }
    };

    class TokenCommand : public Token {
    public:
        TokenCommand(const std::string &cmd) : Token(Token::Type::CMD), command(cmd)
        {
        }

        TokenCommand(const TokenCommand &other) : Token(Token::Type::CMD), command(other.command)
        {
            std::transform(other.arguments.begin(), other.arguments.end(), std::back_inserter(arguments),
                           [](auto &x) { return x->clone(); });
        }

        const std::string command;
        std::vector<std::unique_ptr<Token>> arguments;

        std::unique_ptr<Token> clone() const override
        {
            return std::make_unique<TokenCommand>(*this);
        }
    };

    class TokenString : public Token {
    public:
        TokenString(const std::string &str) : Token(Token::Type::STR), string(str)
        {
        }

        const std::string string;

        std::unique_ptr<Token> clone() const override
        {
            return std::make_unique<TokenString>(*this);
        }
    };

    class TokenUUID : public Token {
    public:
        TokenUUID(const std::string &str) : Token(Token::Type::UUID), string(str)
        {
        }

        const std::string string;

        std::unique_ptr<Token> clone() const override
        {
            return std::make_unique<TokenUUID>(*this);
        }
    };

    using CommandHandler = std::pair<bool, std::string> (ParameterProgram::*)(const TokenCommand &cmd);
    virtual CommandHandler get_command(const std::string &cmd);

    std::vector<int64_t> stack;

private:
    std::string code;

    std::pair<bool, std::string> compile();
    std::pair<bool, std::string> init_error = {false, ""};
    std::vector<std::unique_ptr<Token>> tokens;

    std::pair<bool, std::string> cmd_dump(const TokenCommand &cmd);
    std::pair<bool, std::string> cmd_math1(const TokenCommand &cmd);
    std::pair<bool, std::string> cmd_math2(const TokenCommand &cmd);
    std::pair<bool, std::string> cmd_math3(const TokenCommand &cmd);
};
} // namespace horizon
