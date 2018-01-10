#include "parameter/program.hpp"
using namespace horizon;
#include <iostream>
#include <sstream>
#include <glibmm.h>

class MyParameterProgram: public ParameterProgram {
	using ParameterProgram::ParameterProgram;
	public:
		int i = 5;

	protected:
		std::pair<bool, std::string> put_i(const TokenCommand *cmd, std::deque<int64_t> &stack) {
			stack.push_back(i);
			return {false ,""};
		}

	ParameterProgram::CommandHandler get_command(const std::string &cmd) override {
		if(auto r = ParameterProgram::get_command(cmd)) {
			return r;
		}
		else if(cmd == "lol") {
			using namespace std::placeholders;
			return std::bind(std::mem_fn(&MyParameterProgram::put_i), this, _1, _2);
		}
		else {
			return nullptr;
		}
	}
};


int main(void) {
	std::string line;
	std::stringstream ss;
	while (std::getline(std::cin, line))
	{
		ss << line << std::endl;
	}
	Glib::init();
	MyParameterProgram pgm(ss.str());
	pgm.i = 1337;
	auto err = pgm.get_init_error();
	if(err.first) {
		std::cout << err.second << std::endl;
		exit(1);
	}
	ParameterSet ps = {
		{ParameterID::PAD_WIDTH, 2},
	};
	auto r = pgm.run(ps);
	std::cout << "run " << r.first << " " << r.second << std::endl;
}
