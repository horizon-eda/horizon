#include "gerber_writer.hpp"
#include <iomanip>

namespace horizon {

	GerberWriter::GerberWriter(const std::string &filename): ofs(filename), out_filename(filename) {
		check_open();
	}

	const std::string &GerberWriter::get_filename() {
		return out_filename;
	}

	void GerberWriter::write_line(const std::string &s) {
		check_open();
		ofs << s << std::endl;
	}

	void GerberWriter::check_open() {
		if(!ofs.is_open()) {
			throw std::runtime_error("not opened");
		}
	}

	void GerberWriter::close() {
		write_line("M02*");
		ofs.close();
	}

	void GerberWriter::comment(const std::string &s) {
		if(s.find('*')!=std::string::npos) {
			throw std::runtime_error("comment must not include asterisk");
		}
		ofs << "G04 " << s << "*" << std::endl;
	}

	void GerberWriter::write_format() {
		write_line("%FSLAX46Y46*%");
		write_line("%MOMM*%");
	}

	unsigned int GerberWriter::get_or_create_aperture_circle(uint64_t diameter) {
		if(apertures_circle.count(diameter)) {
			return apertures_circle.at(diameter);
		}
		else {
			auto n = aperture_n++;
			apertures_circle.emplace(diameter, n);
			return n;
		}
	}

	void GerberWriter::write_apertures() {
		ofs.precision(6);
		for(const auto &it: apertures_circle) {
			ofs << "%ADD" << it.second << "C," << std::fixed << (double)it.first/1e6 << "*%" << std::endl;
		}

		/*%AMOUTLINE*
		4,1,3,
		0.1,0.2,
		0.5,0.1,
		0.5,0.5,
		0.1,0.5,
		0.1,0.1,
		0*%
		%ADD18OUTLINE*%
		*/

		for(const auto &it: apertures_polygon) {
			ofs << "%AMPS" << it.second.name << "*" << std::endl;
			ofs << "4,1," << it.second.vertices.size() << "," << std::endl;

			auto write_vertex = [this](const Coordi &c){ofs << std::fixed << (double)c.x/1e6 << "," << (double)c.y/1e6 << "," << std::endl;};
			for(const auto &v: it.second.vertices) {
				write_vertex(v);
			}
			write_vertex(it.second.vertices.front());

			ofs << "0*%" << std::endl;
			ofs << "%ADD" << it.second.name << "PS" << it.second.name << "*%"<< std::endl;
		}
	}

	std::ostream &operator<<(std::ostream &os, const Coordi &c) {
		return os << "X" << c.x << "Y" << c.y;
	}

	void GerberWriter::write_lines() {
		write_line("*G01");
		write_line("%LPD*%");
		for(const auto &it: lines) {
			ofs << "D" << it.aperture << "*" << std::endl;
			ofs << it.from << "D02*" << std::endl;
			ofs << it.to   << "D01*" << std::endl;
		}
	}

	void GerberWriter::write_pads() {
		for(const auto &it: pads) {
			ofs << "D" << it.first << "*" << std::endl;
			ofs << it.second << "D03*" << std::endl;
		}
	}

	void GerberWriter::draw_line(const Coordi &from, const Coordi &to, uint64_t width) {
		auto ap = get_or_create_aperture_circle(width);
		lines.emplace_back(from, to, ap);
	}

	void GerberWriter::draw_padstack(const UUID &padstack_uuid, const Polygon &poly, const Placement &transform) {
		auto key = std::make_tuple(padstack_uuid, transform.get_angle(), transform.mirror);
		GerberWriter::PolygonAperture *ap = nullptr;
		if(apertures_polygon.count(key)) {
			ap = &apertures_polygon.at(key);
		}
		else {
			auto n = aperture_n++;
			ap = &apertures_polygon.emplace(key, n).first->second;
			ap->vertices.reserve(poly.vertices.size());
			auto tr = transform;
			tr.shift = Coordi();
			for(const auto &it: poly.vertices) {
				ap->vertices.push_back(tr.transform(it.position));
			}
		}
		pads.emplace_back(ap->name, transform.shift);
	}
}
