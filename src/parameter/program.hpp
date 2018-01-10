#pragma once
#include <memory>
#include <deque>
#include <string>
#include <functional>
#include "set.hpp"

namespace horizon {
	//using json = nlohmann::json;

	class ParameterProgram {
		friend class ParameterCommands;
		public:
			ParameterProgram(const std::string &s);
			ParameterProgram(const ParameterProgram &other);
			ParameterProgram &operator= (const ParameterProgram &other);
			std::pair<bool, std::string> get_init_error();
			const std::string &get_code() const;
			std::pair<bool, std::string> set_code(const std::string &s);

			std::pair<bool, std::string> run(const ParameterSet &pset={});

		protected:
			class Token {
				public :
					enum class Type {INT, CMD, STR, UUID};
					Token(Type ty): type(ty) {}

					const Type type;

					virtual ~Token() {}
			};

			class TokenInt: public Token {
				public:
					TokenInt(int64_t v): Token(Token::Type::INT), value(v) {}

					const int64_t value;
			};

			class TokenCommand: public Token {
				public:
					TokenCommand(const std::string &cmd): Token(Token::Type::CMD), command(cmd) {}

					const std::string command;
					std::deque<std::unique_ptr<Token>> arguments;
			};

			class TokenString: public Token {
				public:
					TokenString(const std::string &str): Token(Token::Type::STR), string(str) {}

					const std::string string;
			};

			class TokenUUID: public Token {
				public:
					TokenUUID(const std::string &str): Token(Token::Type::UUID), string(str) {}

					const std::string string;
			};
			using CommandHandler = std::function<std::pair<bool, std::string>(const TokenCommand *cmd, std::deque<int64_t> &stack)>;
			virtual CommandHandler get_command(const std::string &cmd);

		private:
			std::string code;
			std::pair<bool, std::string> compile();
			std::pair<bool, std::string> init_error = {false, ""};


			std::deque<std::unique_ptr<Token>> tokens;
	};
}
