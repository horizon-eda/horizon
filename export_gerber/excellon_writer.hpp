#pragma once
#include "common.hpp"
#include <iostream>
#include <fstream>
#include <map>
#include <deque>
#include "padstack.hpp"
#include "placement.hpp"

namespace horizon {

	class ExcellonWriter {
		public:
			ExcellonWriter(const std::string &filename);
			void write_line(const std::string &s);
			void close();
			void write_format();
			void write_header();
			void write_holes();
			void draw_hole(const Coordi &pos, uint64_t diameter);
			const std::string &get_filename();


		private:
			std::map<uint64_t, unsigned int> tools;
			unsigned int tool_n = 1;
			std::deque<std::pair<Coordi, unsigned int>> holes;


			std::ofstream ofs;
			std::string out_filename;
			void check_open();

	};
}
