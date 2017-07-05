#include "error_polygons.hpp"
#include "gl_util.hpp"
#include "canvas.hpp"
#include "line.hpp"

namespace horizon {
	ErrorPolygons::ErrorPolygons(CanvasGL *c): ca(c) {
	}

	void ErrorPolygons::clear_polygons() {
		 ca->triangles.erase(std::remove_if(ca->triangles.begin(),
                              ca->triangles.end(),
                              [](const auto &t){return static_cast<Triangle::Type>(t.type) == Triangle::Type::ERROR;}),
				 ca->triangles.end());
		 ca->request_push();
	}

	void ErrorPolygons::add_polygons(const ClipperLib::Paths &paths) {
		ca->triangle_type_current = Triangle::Type::ERROR;
		for(const auto &path: paths) {
			Coordf last(path.back().X, path.back().Y);
			for(const auto &it: path) {
				Coordf c(it.X, it.Y);
				ca->draw_line(last ,c, ColorP::YELLOW, 10000, false, 0);

				last = c;
			}
		}
		ca->triangle_type_current = Triangle::Type::NONE;
		ca->request_push();
	}

	void ErrorPolygons::set_visible(bool v) {
		ca->set_type_visible(Triangle::Type::ERROR, v);
	}
}
