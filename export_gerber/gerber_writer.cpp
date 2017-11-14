#include "gerber_writer.hpp"
#include "hash.hpp"
#include <iomanip>

namespace horizon {

	GerberWriter::GerberWriter(const std::string &filename): ofs(filename), out_filename(filename) {
		ofs.imbue(std::locale("C"));
		check_open();
	}

	const std::string &GerberWriter::get_filename() {
		return out_filename;
	}

	void GerberWriter::write_line(const std::string &s) {
		check_open();
		ofs << s << "\r\n";
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
		ofs << "G04 " << s << "*" << "\r\n";
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

	void GerberWriter::write_decimal(int64_t x, bool comma) {
		ofs << std::fixed << (double)x/1e6;
		if(comma) {
			ofs << ",";
		}
	}

	void GerberWriter::write_apertures() {
		ofs.precision(6);
		for(const auto &it: apertures_circle) {
			ofs << "%ADD" << it.second << "C," << std::fixed << (double)it.first/1e6 << "*%" << "\r\n";
		}

		for(const auto &it: apertures_macro) {
			ofs << "%AMPS" << it.second.name << "*" << "\r\n";
			for(const auto &itp: it.second.primitives) {
				ofs << static_cast<int>(itp->code) << ",";
				switch(itp->code) {
					case ApertureMacro::Primitive::Code::CIRCLE: {
						auto prim = dynamic_cast<ApertureMacro::PrimitiveCircle*>(itp.get());
						ofs << "1,";  //exposure
						write_decimal(prim->diameter);
						write_decimal(prim->center.x);
						write_decimal(prim->center.y);
						ofs << "0"; //angle
					} break;
					case ApertureMacro::Primitive::Code::CENTER_LINE: {
						auto prim = dynamic_cast<ApertureMacro::PrimitiveCenterLine*>(itp.get());
						ofs << "1,";  //exposure
						write_decimal(prim->width);
						write_decimal(prim->height);

						Placement tr;
						tr.set_angle(-prim->angle);
						auto c = tr.transform(prim->center);
						write_decimal(c.x); //x
						write_decimal(c.y); //y
						ofs << std::fixed << (prim->angle)*(360./65536.);
					} break;
					case ApertureMacro::Primitive::Code::OUTLINE: {
						auto prim = dynamic_cast<ApertureMacro::PrimitiveOutline*>(itp.get());
						assert(prim->vertices.size()>0);
						ofs << "1," << prim->vertices.size() << "," << "\r\n";

						auto write_vertex = [this](const Coordi &c){ofs << std::fixed << (double)c.x/1e6 << "," << (double)c.y/1e6 << "," << "\r\n";};
						for(const auto &v: prim->vertices) {
							write_vertex(v);
						}
						write_vertex(prim->vertices.front());
						ofs << "0" << "\r\n";


					} break;
				}
				ofs << "*" << "\r\n";

			}


			ofs << "%" << "\r\n";
			ofs << "%ADD" << it.second.name << "PS" << it.second.name << "*%"<< "\r\n";
		}
	}

	std::ostream &operator<<(std::ostream &os, const Coordi &c) {
		return os << "X" << c.x << "Y" << c.y;
	}

	void GerberWriter::write_lines() {
		write_line("G01*");
		write_line("%LPD*%");
		for(const auto &it: lines) {
			ofs << "D" << it.aperture << "*" << "\r\n";
			ofs << it.from << "D02*" << "\r\n";
			ofs << it.to   << "D01*" << "\r\n";
		}
	}

	void GerberWriter::write_pads() {
		for(const auto &it: pads) {
			ofs << "D" << it.first << "*" << "\r\n";
			ofs << it.second << "D03*" << "\r\n";
		}
	}

	void GerberWriter::write_regions() {
		write_line("G01*");
		for(const auto &it: regions) {
			if(it.dark) {
				write_line("%LPD*%");
			}
			else {
				write_line("%LPC*%");
			}
			write_line("G36*");
			ofs << Coordi(it.path.back().X, it.path.back().Y) << "D02*" << "\r\n";
			for(const auto &pt: it.path) {
				ofs << Coordi(pt.X, pt.Y) << "D01*" << "\r\n";
			}
			write_line("D02*");
			write_line("G37*");
		}
	}

	void GerberWriter::draw_line(const Coordi &from, const Coordi &to, uint64_t width) {
		auto ap = get_or_create_aperture_circle(width);
		lines.emplace_back(from, to, ap);
	}

	void GerberWriter::draw_region(const ClipperLib::Path &path, bool dark) {
		regions.emplace_back(path, dark);
	}

	void GerberWriter::draw_padstack(const Padstack &ps, int layer, const Placement &transform) {
		auto hash = GerberHash::hash(ps);
		//the hash thing is needed since we have parameters and the same uuid doens't imply the same padstack anymore
		auto key = std::make_tuple(ps.uuid, hash, transform.get_angle(), transform.mirror);
		ApertureMacro *am = nullptr;
		if(apertures_macro.count(key)) {
			am = &apertures_macro.at(key);
		}
		else {
			auto n = aperture_n++;
			am = &apertures_macro.emplace(key, n).first->second;
			auto tr = transform;
			tr.shift = Coordi();
			for(const auto &it: ps.shapes) {
				if(it.second.layer == layer) {
					switch(it.second.form) {
						case Shape::Form::CIRCLE : {
							am->primitives.push_back(std::make_unique<ApertureMacro::PrimitiveCircle>());
							auto prim = dynamic_cast<ApertureMacro::PrimitiveCircle*>(am->primitives.back().get());
							prim->diameter = it.second.params.at(0);
							prim->center = tr.transform(it.second.placement.shift);
						} break;

						case Shape::Form::RECTANGLE: {
							am->primitives.push_back(std::make_unique<ApertureMacro::PrimitiveCenterLine>());
							auto prim = dynamic_cast<ApertureMacro::PrimitiveCenterLine*>(am->primitives.back().get());
							prim->width = it.second.params.at(0);
							prim->height = it.second.params.at(1);
							auto tr2 = tr;
							tr2.accumulate(it.second.placement);
							prim->center = tr2.shift;
							prim->angle = tr2.get_angle();
						} break;

						case Shape::Form::OBROUND: {
							am->primitives.push_back(std::make_unique<ApertureMacro::PrimitiveCenterLine>());
							auto prim = dynamic_cast<ApertureMacro::PrimitiveCenterLine*>(am->primitives.back().get());
							prim->height = it.second.params.at(1);
							prim->width = it.second.params.at(0)-prim->height;
							auto tr2 = tr;
							tr2.accumulate(it.second.placement);
							prim->center = tr2.shift;
							prim->angle = tr2.get_angle();

							am->primitives.push_back(std::make_unique<ApertureMacro::PrimitiveCircle>());
							auto prim_c1 = dynamic_cast<ApertureMacro::PrimitiveCircle*>(am->primitives.back().get());
							prim_c1->diameter = prim->height;
							prim_c1->center = tr2.transform(Coordi(prim->width/2, 0));

							am->primitives.push_back(std::make_unique<ApertureMacro::PrimitiveCircle>());
							auto prim_c2 = dynamic_cast<ApertureMacro::PrimitiveCircle*>(am->primitives.back().get());
							prim_c2->diameter = prim->height;
							prim_c2->center = tr2.transform(Coordi(-prim->width/2, 0));
						} break;
					}

				}
			}
			for(const auto &it: ps.polygons) {
				if(it.second.layer == layer) {
					if(it.second.vertices.size()) {
						am->primitives.push_back(std::make_unique<ApertureMacro::PrimitiveOutline>());
						auto prim = dynamic_cast<ApertureMacro::PrimitiveOutline*>(am->primitives.back().get());
						for(const auto &itv: it.second.vertices) {
							prim->vertices.push_back(tr.transform(itv.position));
						}
					}
				}
			}
		}
		pads.emplace_back(am->name, transform.shift);
	}
}
