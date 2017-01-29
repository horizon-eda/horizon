#pragma once
#include "common.hpp"
#include <iostream>
#include <fstream>
#include <map>
#include <deque>
#include "padstack.hpp"
#include "placement.hpp"

namespace horizon {

	class GerberWriter {
		public:
			GerberWriter(const std::string &filename);
			void write_line(const std::string &s);
			void close();
			void comment(const std::string &s);
			void write_format();
			void write_apertures();
			void write_lines();
			void write_pads();
			unsigned int get_or_create_aperture_circle(uint64_t diameter);
			//unsigned int get_or_create_aperture_padstack(const class Padstack *ps, int layer, )
			void draw_line(const Coordi &from, const Coordi &to, uint64_t width);
			void draw_padstack(const UUID &padstack_uuid, const Polygon &poly, const Placement &transform);
			const std::string &get_filename();

		private:
			class Line {
				public:
					Line(const Coordi &f, const Coordi &t, unsigned int ap): from(f), to(t), aperture(ap) {}
					Coordi from;
					Coordi to;
					unsigned int aperture;
			};

			class PolygonAperture {
				public:
					PolygonAperture(unsigned int n):name(n){}

					unsigned int name;
					std::vector<Coordi> vertices;
			};

			std::ofstream ofs;
			std::string out_filename;
			void check_open();
			std::map<uint64_t, unsigned int> apertures_circle;
			std::map<std::tuple<UUID, int, bool>, PolygonAperture> apertures_polygon;

			unsigned int aperture_n=10;

			std::deque<Line> lines;
			std::deque<std::pair<unsigned int, Coordi>> pads;
			void write_coord(const Coordi &c);
	};
}
