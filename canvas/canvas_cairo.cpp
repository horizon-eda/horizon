#include "canvas_cairo.hpp"

namespace horizon {
		CanvasCairo::CanvasCairo(Cairo::RefPtr<Cairo::Context> c) : Canvas::Canvas(), cr(c) {}
		void CanvasCairo::request_push() {
			cr->save();
			cr->scale(2.83465, -2.83465);
			cr->translate(0, -209.9);
			cr->set_source_rgb(0,0,0);
			cr->set_line_width(.05);
			cr->set_line_cap(Cairo::LINE_CAP_ROUND);
			for(const auto &it: triangles) {
				if(isnan(it.y2)) {
					cr->set_line_width(std::max(it.x2/1e6, .05));
					cr->move_to(it.x0/1e6,it.y0/1e6);
					cr->line_to(it.x1/1e6,it.y1/1e6);
					cr->stroke();
				}
			}
			cr->restore();
		}
}
