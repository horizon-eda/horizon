#include "excellon_writer.hpp"
#include <iomanip>

namespace horizon {

	ExcellonWriter::ExcellonWriter(const std::string &filename): ofs(filename), out_filename(filename) {
		ofs.imbue(std::locale("C"));
		check_open();
	}

	const std::string &ExcellonWriter::get_filename() {
		return out_filename;
	}

	void ExcellonWriter::write_line(const std::string &s) {
		check_open();
		ofs << s << "\r\n";
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
			ofs << "T" << it.second << "C" << std::fixed << (double)it.first/1e6  << "\r\n";
		}
		write_line("%");
		write_line("G90");
		write_line("G05");
		write_line("M71");
	}

	unsigned int ExcellonWriter::get_tool_for_diameter(uint64_t diameter) {
		unsigned int tool;
		if(tools.count(diameter)) {
			tool = tools.at(diameter);
		}
		else {
			tool = tool_n++;
			tools.emplace(diameter, tool);
		}
		return tool;
	}

	void ExcellonWriter::draw_hole(const Coordi &pos, uint64_t diameter) {
		unsigned int tool = get_tool_for_diameter(diameter);
		holes.emplace_back(pos, tool);
	}

	void ExcellonWriter::draw_slot(const Coordi &pos, uint64_t diameter, uint64_t length, int angle) {
		unsigned int tool = get_tool_for_diameter(diameter);
		double d = std::max(((int64_t)length - (int64_t)diameter)/2, (int64_t)0);
		double phi = (angle/65536.0)*2*M_PI;
		double dx = d*cos(phi);
		double dy = d*sin(phi);

		Coordi p0(pos.x-dx, pos.y-dy);
		Coordi p1(pos.x+dx, pos.y+dy);
		slots.emplace_back(p0, p1, tool);
	}

	void ExcellonWriter::write_holes() {
		ofs.precision(3);
		for(const auto &itt: tools) {
			auto tool = itt.second;
			ofs << "T" << tool << "\r\n";
			for(const auto &it: holes) {
				if(it.second == tool)
					ofs << "X" << std::fixed << (double)it.first.x/1e6 << "Y" << std::fixed << (double)it.first.y/1e6 << "\r\n";
			}
			for(const auto &it: slots) {
				Coordi p0;
				Coordi p1;
				unsigned int this_tool;
				std::tie(p0, p1, this_tool) = it;
				if(this_tool == tool)
					ofs << "X" << std::fixed << (double)p0.x/1e6 << "Y" << std::fixed << (double)p0.y/1e6 << "G85" << "X" << std::fixed << (double)p1.x/1e6 << "Y" << std::fixed << (double)p1.y/1e6 << "\r\n";
			}
		}

	}


}
