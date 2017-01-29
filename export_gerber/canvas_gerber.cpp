#include "canvas_gerber.hpp"
#include "core/core_board.hpp"
#include "gerber_export.hpp"

namespace horizon {
		CanvasGerber::CanvasGerber(GerberExporter *exp) : Canvas::Canvas(), exporter(exp) {
			img_mode = true;
		}
		void CanvasGerber::request_push() {}

		void CanvasGerber::img_net(const Net *n) {

		}

		void CanvasGerber::img_polygon(const Polygon &ipoly) {
			auto poly = ipoly.remove_arcs(16);
			if(poly.layer == 50) { //outline
				auto last = poly.vertices.cbegin();
				for(auto it = last+1; it<poly.vertices.cend(); it++) {
					img_line(last->position, it->position, 0, poly.layer);
					last = it;
				}
				img_line(poly.vertices.front().position, poly.vertices.back().position, 0, poly.layer);
			}

		}

		void CanvasGerber::img_line(const Coordi &p0, const Coordi &p1, const uint64_t width, int layer) {
			if(GerberWriter *wr = exporter->get_writer_for_layer(layer)) {
				wr->draw_line(transform.transform(p0), transform.transform(p1), width);
			}

		}

		void CanvasGerber::img_padstack(const Padstack &padstack) {
			for(const auto &it: padstack.polygons) {
				auto poly = it.second.remove_arcs(32);
				if(GerberWriter *wr = exporter->get_writer_for_layer(poly.layer)) {
					wr->draw_padstack(padstack.uuid, poly, transform);
				}
			}
		}

		void CanvasGerber::img_hole(const Hole &hole) {
			auto wr = exporter->get_drill_writer(hole.plated);
			wr->draw_hole(transform.transform(hole.position), hole.diameter);
		}


}
