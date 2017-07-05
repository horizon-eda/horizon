#include "canvas.hpp"
#include <iostream>
#include <algorithm>
#include "board/board.hpp"
#include "common/layer_provider.hpp"

namespace horizon {

	Canvas::Canvas(): selection_filter(this), selectables(this) {
		//RED, GREEN, YELLOW, WHITE, ERROR, NET, BUS, SYMBOL, FRAME, AIRWIRE, LAST}
		layer_setup.colors.at(static_cast<int>(ColorP::RED)) = {1,0,0};
		layer_setup.colors.at(static_cast<int>(ColorP::GREEN)) = {0,1,0};
		layer_setup.colors.at(static_cast<int>(ColorP::YELLOW)) = {1,1,0};
		layer_setup.colors.at(static_cast<int>(ColorP::WHITE)) = {1,1,1};
		layer_setup.colors.at(static_cast<int>(ColorP::ERROR)) = {1,0,0};
		layer_setup.colors.at(static_cast<int>(ColorP::NET)) = {0,1,0};
		layer_setup.colors.at(static_cast<int>(ColorP::BUS)) = {1,.4,0};
		layer_setup.colors.at(static_cast<int>(ColorP::SYMBOL)) = {1,1,1};
		layer_setup.colors.at(static_cast<int>(ColorP::FRAME)) = {0,.5,0};
		layer_setup.colors.at(static_cast<int>(ColorP::AIRWIRE)) = {0,1,1};
	}

	void Canvas::set_layer_display(int index, const LayerDisplay &ld) {
		layer_setup.layer_display.at(compress_layer(index)) = ld;
		request_push();
	}

	LayerDisplayGL::LayerDisplayGL(const LayerDisplay &ld): r(ld.color.r), g(ld.color.g), b(ld.color.b),
		flags(ld.visible|((static_cast<int>(ld.mode)&3)<<1)) {}

	LayerDisplayGL::LayerDisplayGL() {}

	void Canvas::clear() {
		selectables.clear();
		triangles.erase(std::remove_if(triangles.begin(),
						triangles.end(),
						[](const auto &t){return static_cast<Triangle::Type>(t.type) != Triangle::Type::ERROR;}), triangles.end());
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

