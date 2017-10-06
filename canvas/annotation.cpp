#include "annotation.hpp"
#include "layer_display.hpp"
#include "canvas.hpp"

namespace horizon {
	CanvasAnnotation *Canvas::create_annotation() {
		annotation_layer_current++;
		return &annotations.emplace(std::piecewise_construct, std::forward_as_tuple(annotation_layer_current), std::forward_as_tuple(this, annotation_layer_current)).first->second;
	}

	void Canvas::remove_annotation(CanvasAnnotation *a) {
		auto layer = a->layer;
		annotations.erase(layer);
		if(triangles.count(layer))
			triangles.erase(layer);
	}

	CanvasAnnotation::CanvasAnnotation(Canvas *c, int l): ca(c), layer(l) {
		LayerDisplay ld(false, LayerDisplay::Mode::FILL_ONLY, {1,1,0});
		ca->set_layer_display(layer, ld);
	}

	void CanvasAnnotation::set_visible(bool v) {
		ca->layer_display[layer].visible = v;
	}

	void CanvasAnnotation::set_display(const LayerDisplay &ld) {
		ca->set_layer_display(layer, ld);
	}

	void CanvasAnnotation::clear() {
		if(ca->triangles.count(layer))
			ca->triangles[layer].clear();
	}

	void CanvasAnnotation::draw_line(const Coordi &from, const Coordi &to, ColorP color, uint64_t width) {
		ca->draw_line(from, to, color, layer, false, width);
	}
}
