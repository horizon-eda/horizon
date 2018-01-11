#pragma once
#include "common/common.hpp"
#include <iostream>
#include <fstream>
#include <map>
#include <deque>
#include "pool/padstack.hpp"
#include "util/placement.hpp"

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
			void draw_slot(const Coordi &pos, uint64_t diameter, uint64_t length, int angle);
			const std::string &get_filename();


		private:
			std::map<uint64_t, unsigned int> tools;
			unsigned int tool_n = 1;
			unsigned int get_tool_for_diameter(uint64_t dia);

			std::deque<std::pair<Coordi, unsigned int>> holes;
			std::deque<std::tuple<Coordi, Coordi, unsigned int>> slots;


			std::ofstream ofs;
			std::string out_filename;
			void check_open();

	};
}
