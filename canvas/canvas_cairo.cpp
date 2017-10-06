#include "canvas_cairo.hpp"
#include <cmath>

namespace horizon {
		CanvasCairo::CanvasCairo(Cairo::RefPtr<Cairo::Context> c) : Canvas::Canvas(), cr(c) {}
		void CanvasCairo::request_push() {
			cr->save();

			cr->set_source_rgb(0,0,0);
			cr->set_line_width(.05);
			cr->set_line_cap(Cairo::LINE_CAP_ROUND);
			for(const auto &it: triangles) {
				for(const auto &it2: it.second) {
					if(std::isnan(it2.y2)) {
						cr->set_line_width(std::max(it2.x2/1e6, .05));
						cr->move_to(it2.x0/1e6,it2.y0/1e6);
						cr->line_to(it2.x1/1e6,it2.y1/1e6);
						cr->stroke();
					}
					else {
						cr->move_to(it2.x0/1e6,it2.y0/1e6);
						cr->line_to(it2.x1/1e6,it2.y1/1e6);
						cr->line_to(it2.x2/1e6,it2.y2/1e6);
						cr->close_path();
						cr->fill();
					}
				}
			}
			cr->restore();
		}
}
