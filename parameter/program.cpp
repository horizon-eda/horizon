#include "program.hpp"
#include <sstream>
#include <glibmm.h>
#include <stdint.h>
#include <iostream>
#include <map>


namespace horizon {
	ParameterProgram::ParameterProgram(const std::string &s):code(s) {
		init_error = compile();
	}

	ParameterProgram::ParameterProgram(const ParameterProgram &other): code(other.code) {
		init_error = compile();
	}

	ParameterProgram &ParameterProgram::operator= (const ParameterProgram &other) {
		code = other.code;
		init_error = compile();
		return *this;
	}

	std::pair<bool, std::string> ParameterProgram::get_init_error() {
		return init_error;
	}

	const std::string &ParameterProgram::get_code() const {
		return code;
	}

	std::pair<bool, std::string> ParameterProgram::set_code(const std::string &s) {
		code = s;
		return compile();
	}

	static bool stack_pop(std::deque<int64_t> &stack, int64_t &va) {
		if(stack.size()) {
			va = stack.back();
			stack.pop_back();
			return false;
		}
		else {
			return true;
		}
	}

	class ParameterCommands {
		public:

		static std::pair<bool, std::string> dump(const ParameterProgram::TokenCommand *cmd, std::deque<int64_t> &stack) {
			auto sz = stack.size();
			for(const auto &it: stack) {
				sz--;
				std::cout << sz << ": " << it << "\n";
			}
			std::cout << std::endl;
			return {false, ""};
		}

		static std::pair<bool, std::string> math1(const ParameterProgram::TokenCommand *cmd, std::deque<int64_t> &stack) {
			int64_t a;
			if(stack_pop(stack, a))
				return {true, "empty stack"};
			if(cmd->command == "dup") {
				stack.push_back(a);
				stack.push_back(a);
			}
			return {false, ""};
		}

		static std::pair<bool, std::string> math3(const ParameterProgram::TokenCommand *cmd, std::deque<int64_t> &stack) {
			int64_t a,b,c;
			if(stack_pop(stack, c) || stack_pop(stack, b) || stack_pop(stack, a))
				return {true, "empty stack"};
			if(cmd->command == "+xy") {
				stack.push_back(a+c);
				stack.push_back(b+c);
			}
			else if(cmd->command == "-xy") {
				stack.push_back(a-c);
				stack.push_back(b-c);
			}
			return {false, ""};
		}

		static std::pair<bool, std::string> math2(const ParameterProgram::TokenCommand *cmd, std::deque<int64_t> &stack) {
			int64_t a,b;
			if(stack_pop(stack, b) || stack_pop(stack, a))
				return {true, "empty stack"};
			if(cmd->command[0] == '+') {
				stack.push_back(a+b);
			}
			else if(cmd->command[0] == '-') {
				stack.push_back(a-b);
			}
			else if(cmd->command[0] == '*') {
				stack.push_back(a*b);
			}
			else if(cmd->command[0] == '/') {
				stack.push_back(a/b);
			}
			else if(cmd->command == "dupc") {
				stack.push_back(a);
				stack.push_back(b);
				stack.push_back(a);
				stack.push_back(b);
			}
			else if(cmd->command == "swap") {
				stack.push_back(b);
				stack.push_back(a);
			}
			return {false, ""};
		}

		static std::function<std::pair<bool, std::string>(const ParameterProgram::TokenCommand *cmd, std::deque<int64_t> &stack)> get_command(const std::string &cmd) {
			static const std::map<std::string, std::function<std::pair<bool, std::string>(const ParameterProgram::TokenCommand *cmd, std::deque<int64_t> &stack)>> commands = {
				{"dump",	&dump},
				{"+", 		&math2},
				{"-", 		&math2},
				{"*", 		&math2},
				{"/", 		&math2},
				{"dup",		&math1},
				{"dupc",	&math2},
				{"swap",	&math2},
				{"+xy",		&math3},
				{"-xy",		&math3},
			};

			if(commands.count(cmd))
				return commands.at(cmd);
			else
				return nullptr;
		}
	};

	std::function<std::pair<bool, std::string>(const ParameterProgram::TokenCommand *cmd, std::deque<int64_t> &stack)> ParameterProgram::get_command(const std::string &cmd) {
		return ParameterCommands::get_command(cmd);
	}

	std::pair<bool, std::string> ParameterProgram::run(const ParameterSet &pset) {
		std::deque<int64_t> stack;
		for(const auto &token: tokens) {
			switch(token->type) {
				case Token::Type::CMD : {
					auto tok = dynamic_cast<TokenCommand*>(token.get());
					if(auto cmd = get_command(tok->command)) {
						auto r = cmd(tok, stack);
						if(r.first) {
							return r;
						}
					}
					else if(tok->command == "get-parameter") {
						if(tok->arguments.size() < 1 || tok->arguments.at(0)->type != Token::Type::STR) {
							return {true, "get-parameter requires one string argument"};
						}
						auto arg = dynamic_cast<TokenString*>(tok->arguments.at(0).get());
						ParameterID pid = parameter_id_from_string(arg->string);
						if(pid == ParameterID::INVALID) {
							return {true, "invalid parameter "+arg->string};
						}
						if(pset.count(pid) == 0) {
							return {true, "parameter not found: "+parameter_id_to_string(pid)};
						}
						stack.push_back(pset.at(pid));
					}
					else {
						return {true, "unknown command " + tok->command};
					}
				} break;
				case Token::Type::INT : {
					auto tok = dynamic_cast<TokenInt*>(token.get());
					stack.push_back(tok->value);
				} break;
			}
		}

		return {false, ""};
	}

	std::pair<bool, std::string> ParameterProgram::compile() {
		std::stringstream iss(code);
		std::deque<std::string> stokens{std::istream_iterator<std::string>{iss},
						  std::istream_iterator<std::string>{}};

		const auto regex_int = Glib::Regex::create("^([+-]?\\d+)$");
		const auto regex_dim = Glib::Regex::create("^([+-]?(?:\\d*\\.)?\\d+)mm$");
		const auto regex_str = Glib::Regex::create("^([a-z][a-z-_0-9]*)$");
		const auto regex_math = Glib::Regex::create("^([+-/\x2A][a-z]*)$");
		const auto regex_uuid = Glib::Regex::create("^[0-9a-f]{8}-[0-9a-f]{4}-[1-5][0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$");
		tokens.clear();

		bool arg_mode = false;
		for(const auto &it: stokens) {
			Glib::ustring token(it);
			Glib::MatchInfo ma;
			//std::cout << "tok " << it << std::endl;
			std::deque<std::unique_ptr<Token>> &ts = arg_mode?dynamic_cast<TokenCommand*>(tokens.back().get())->arguments:tokens;
			if(regex_math->match(token, ma)) {
				tokens.push_back(std::make_unique<TokenCommand>(ma.fetch(1)));
			}
			else if(regex_int->match(token, ma)) {
				ts.push_back(std::make_unique<TokenInt>(std::stoi(ma.fetch(1))));
			}
			else if(regex_dim->match(token, ma)) {
				double f;
				std::istringstream istr(ma.fetch(1));
				istr.imbue(std::locale("C"));
				istr >> f;
				ts.push_back(std::make_unique<TokenInt>(1e6*f));
			}
			else if(regex_str->match(token, ma) && !arg_mode) {
				tokens.push_back(std::make_unique<TokenCommand>(ma.fetch(1)));
			}
			else if(regex_uuid->match(token, ma) && arg_mode) {
				ts.push_back(std::make_unique<TokenUUID>(ma.fetch(1)));
			}
			else if(regex_str->match(token, ma) && arg_mode) {
				ts.push_back(std::make_unique<TokenString>(ma.fetch(1)));
			}
			else if(token == "[") {
				if(arg_mode == true) {
					return {true, "repeated ["};
				}
				if(tokens.back()->type != Token::Type::CMD) {
					return {true, "[ has to follow command token"};
				}
				arg_mode = true;
			}
			else if(token == "]") {
				if(arg_mode == false) {
					return {true, "repeated ]"};
				}
				arg_mode = false;
			}
			else {
				return {true, "unhandled token " + token};
			}
		}
		return {false, ""};
	}
}
