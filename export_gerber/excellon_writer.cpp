#include "excellon_writer.hpp"
#include <iomanip>

namespace horizon {

	ExcellonWriter::ExcellonWriter(const std::string &filename): ofs(filename), out_filename(filename) {
		check_open();
	}

	const std::string &ExcellonWriter::get_filename() {
		return out_filename;
	}

	void ExcellonWriter::write_line(const std::string &s) {
		check_open();
		ofs << s << std::endl;
	}

	void ExcellonWriter::check_open() {
		if(!ofs.is_open()) {
			throw std::runtime_error("not opened");
		}
	}

	void ExcellonWriter::close() {
		write_line("M30");
		ofs.close();
	}

	void ExcellonWriter::write_format() {
		write_line("M48");
		write_line("FMAT,2");
		write_line("METRIC,TZ");
	}

	void ExcellonWriter::write_header() {
		ofs.precision(3);
		for(const auto &it: tools) {
			ofs << "T" << it.second << "C" << std::fixed << (double)it.first/1e6  << std::endl;
		}
		write_line("%");
		write_line("G90");
		write_line("G05");
		write_line("M71");
	}

	void ExcellonWriter::draw_hole(const Coordi &pos, uint64_t diameter) {
		unsigned int tool;
		if(tools.count(diameter)) {
			tool = tools.at(diameter);
		}
		else {
			tool = tool_n++;
			tools.emplace(diameter, tool);
		}
		holes.emplace_back(pos, tool);
	}

	void ExcellonWriter::write_holes() {
		ofs.precision(3);
		for(const auto &it: holes) {
			ofs << "T" << it.second << std::endl;
			ofs << "X" << std::fixed << (double)it.first.x/1e6 << "Y" << std::fixed << (double)it.first.y/1e6 << std::endl;
		}
	}


}
