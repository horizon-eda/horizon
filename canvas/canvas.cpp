#include "canvas.hpp"
#include <iostream>
#include <algorithm>
#include "board/board.hpp"
#include "common/layer_provider.hpp"

namespace horizon {

	Canvas::Canvas(): selection_filter(this), selectables(this) {}

	void Canvas::set_layer_display(int index, const LayerDisplay &ld) {
		layer_display[index] = ld;
	}

	void Canvas::clear() {
		selectables.clear();
		triangles.clear();
		targets.clear();
		sheet_current_uuid = UUID();
	}

	void Canvas::update(const Symbol &sym) {
		clear();
		layer_provider = &sym;
		render(sym);
		request_push();
	}
	
	void Canvas::update(const Sheet &sheet) {
		clear();
		layer_provider = &sheet;
		sheet_current_uuid = sheet.uuid;
		update_markers();
		render(sheet);
		request_push();
	}

	void Canvas::update(const Padstack &padstack) {
		clear();
		layer_provider = &padstack;
		render(padstack);
		request_push();
	}

	void Canvas::update(const Package &pkg) {
		clear();
		layer_provider = &pkg;
		render(pkg);
		request_push();
	}

	void Canvas::update(const Buffer &buf, LayerProvider *lp) {
		clear();
		layer_provider = lp;
		render(buf);
		request_push();
	}
	void Canvas::update(const Board &brd) {
		clear();
		layer_provider = &brd;
		render(brd);
		request_push();
	}

	void Canvas::transform_save() {
		transforms.push_back(transform);
	}

	void Canvas::transform_restore() {
		if(transforms.size()) {
			transform = transforms.back();
			transforms.pop_back();
		}
	}
}

