#include "canvas.hpp"
#include <iostream>
#include <algorithm>
#include "board/board.hpp"
#include "common/layer_provider.hpp"
#include "util.hpp"

namespace horizon {

	Canvas::Canvas(): selection_filter(this), selectables(this) {
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
		layer_setup.colors.at(static_cast<int>(ColorP::PIN)) = {1,1,1};
		layer_setup.colors.at(static_cast<int>(ColorP::PIN_HIDDEN)) = {.5,.5,.5};

		layer_display[10000] = LayerDisplay(true, LayerDisplay::Mode::FILL, {1,1,1});
	}

	void Canvas::set_layer_display(int index, const LayerDisplay &ld) {
		layer_display[index] = ld;
	}

	void Canvas::clear() {
		selectables.clear();
		map_erase_if(triangles, [](auto &x){return x.first<20000;});
		targets.clear();
		sheet_current_uuid = UUID();
		clear_oids();
	}

	void Canvas::set_oid(const SelectableRef &r) {
		if(oid_map.count(r)) {
			oid_current = oid_map.at(r);
		}
		else {
			oid_max++;
			oid_current = oid_max;
			oid_map[r] = oid_current;
		}
	}

	void Canvas::unset_oid() {
		oid_current = 0;
	}

	void Canvas::clear_oids() {
		oid_map.clear();
		oid_max = 1;
		oid_current = 1;
	}

	void Canvas::remove_obj(const SelectableRef &r) {
		auto oid = oid_map.at(r);
		for(auto &it: triangles) {
			auto &v = it.second;
			v.erase(std::remove_if(v.begin(),  v.end(), [oid](auto &x){return x.oid==oid;}), v.end());
		}
		request_push();
	}

	void Canvas::hide_obj(const SelectableRef &r) {
		auto oid = oid_map.at(r);
		std::cout << "hide oid " << oid <<std::endl;
		for(auto &it: triangles) {
			for(auto &it2: it.second)  {
				if(it2.oid == oid)
					it2.flags |= Triangle::FLAG_HIDDEN;
			}
		}
		request_push();
	}

	void Canvas::show_obj(const SelectableRef &r) {
		auto oid = oid_map.at(r);
		for(auto &it: triangles) {
			for(auto &it2: it.second)  {
				if(it2.oid == oid)
					it2.flags &= ~Triangle::FLAG_HIDDEN;
			}
		}
		request_push();
	}

	void Canvas::show_all_obj() {
		for(auto &it: triangles) {
			for(auto &it2: it.second)  {
				it2.flags &= ~Triangle::FLAG_HIDDEN;
			}
		}
		request_push();
	}



	void Canvas::update(const Symbol &sym, const Placement &tr) {
		clear();
		layer_provider = &sym;
		transform = tr;
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

