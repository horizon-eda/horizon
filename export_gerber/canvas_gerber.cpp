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
			if(padstack_mode)
				return;
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

		void CanvasGerber::img_line(const Coordi &p0, const Coordi &p1, const uint64_t width, int layer, bool tr) {
			if(GerberWriter *wr = exporter->get_writer_for_layer(layer)) {
				if(tr)
					wr->draw_line(transform.transform(p0), transform.transform(p1), width);
				else
					wr->draw_line(p0, p1, width);
			}

		}

		void CanvasGerber::img_padstack(const Padstack &padstack) {
			std::set<int> layers;
			for(const auto &it: padstack.polygons) {
				layers.insert(it.second.layer);
			}
			for(const auto &it: padstack.shapes) {
				layers.insert(it.second.layer);
			}
			for(const auto layer: layers) {
				if(GerberWriter *wr = exporter->get_writer_for_layer(layer)) {
					wr->draw_padstack(padstack, layer, transform);
				}
			}
		}

		void CanvasGerber::img_set_padstack(bool v) {
			padstack_mode = v;
		}

		void CanvasGerber::img_hole(const Hole &hole) {
			auto wr = exporter->get_drill_writer(hole.plated);
			if(hole.shape == Hole::Shape::ROUND)
				wr->draw_hole(transform.transform(hole.placement.shift), hole.diameter);
			else if(hole.shape == Hole::Shape::SLOT) {
				auto tr = transform;
				tr.accumulate(hole.placement);
				wr->draw_slot(tr.shift, hole.diameter, hole.length, tr.get_angle());
			}
		}


}
