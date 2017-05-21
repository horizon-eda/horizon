#include "error_polygons.hpp"
#include "gl_util.hpp"
#include "canvas.hpp"

namespace horizon {
	ErrorPolygons::ErrorPolygons(CanvasGL *c): ca(c) {
	}

	void ErrorPolygons::clear_polygons() {
		triangles.clear();
	}

	void ErrorPolygons::add_polygons(const ClipperLib::Paths &paths) {
		for(const auto &path: paths) {
			Coordf last(path.back().X, path.back().Y);
			for(const auto &it: path) {
				Coordf c(it.X, it.Y);
				triangles.emplace_back(last, c, Coordf(0, NAN), Color(1, 0, 1), 0);
				last = c;
			}
		}
	}

	void ErrorPolygons::set_visible(bool v) {
		visible = v;
		ca->request_push();
	}
}
