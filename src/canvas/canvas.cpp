#include "canvas.hpp"
#include <iostream>
#include <algorithm>
#include "board/board.hpp"
#include "common/layer_provider.hpp"
#include "util/util.hpp"
#include "schematic/sheet.hpp"

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
		layer_setup.colors.at(static_cast<int>(ColorP::DIFFPAIR)) = {.5,1,0};

		layer_display[10000] = LayerDisplay(true, LayerDisplay::Mode::FILL, {1,1,1});
	}

	void Canvas::set_layer_display(int index, const LayerDisplay &ld) {
		layer_display[index] = ld;
	}

	static const LayerDisplay ld_default;

	const LayerDisplay &Canvas::get_layer_display(int index) {
		if(layer_display.count(index))
			return layer_display.at(index);
		else
			return ld_default;
	}

	bool Canvas::layer_is_visible(int layer) const {
		return layer == work_layer || layer_display.at(layer).visible;
	}

	void Canvas::clear() {
		selectables.clear();
		for(auto &it: triangles) {
			if(it.first<20000 || it.first >= 30000)
				it.second.clear();
		}
		targets.clear();
		sheet_current_uuid = UUID();
		object_refs.clear();
		object_refs_current.clear();
	}

	void Canvas::remove_obj(const ObjectRef &r) {
		if(!object_refs.count(r))
			return;
		std::set<int> layers;
		for(const auto &it: object_refs.at(r)) {
			auto layer = it.first;
			layers.insert(layer);
			for(const auto &idxs: it.second) {
				auto begin = triangles[layer].begin();
				auto first = begin+idxs.first;
				auto last = begin+idxs.second+1;
				triangles[layer].erase(first, last);
			}
		}

		//fix refs that changed due to triangles being deleted
		for(auto &it: object_refs) {
			if(it.first != r) {
				for(auto layer: layers) {
					if(it.second.count(layer)) {
						auto &idxs = it.second.at(layer);
						for(const auto &idx_removed: object_refs.at(r).at(layer)) {
							auto n_removed = (idx_removed.second-idx_removed.first)+1;
							for(auto &idx: idxs) {
								if(idx.first > idx_removed.first) {
									idx.first -= n_removed;
									idx.second -= n_removed;
								}
							}
						}
					}
				}
			}
		}
		object_refs.erase(r);

		request_push();
	}

	void Canvas::set_flags(const ObjectRef &r, uint8_t mask_set, uint8_t mask_clear) {
		if(!object_refs.count(r))
			return;
		for(const auto &it: object_refs.at(r)) {
			auto layer = it.first;
			for(const auto &idxs: it.second) {
				for(auto i = idxs.first; i<=idxs.second; i++) {
					triangles[layer][i].flags |= mask_set;
					triangles[layer][i].flags &= ~mask_clear;
				}
			}
		}
		request_push();
	}

	void Canvas::set_flags_all(uint8_t mask_set, uint8_t mask_clear) {
		for(auto &it: triangles) {
			for(auto &it2: it.second)  {
				it2.flags |= mask_set;
				it2.flags &= ~mask_clear;
			}
		}
		request_push();
	}

	void Canvas::hide_obj(const ObjectRef &r) {
		set_flags(r, Triangle::FLAG_HIDDEN, 0);
	}

	void Canvas::show_obj(const ObjectRef &r) {
		set_flags(r, 0, Triangle::FLAG_HIDDEN);
	}

	void Canvas::show_all_obj() {
		set_flags_all(0, Triangle::FLAG_HIDDEN);
	}

	void Canvas::add_triangle(int layer, const Coordf &p0, const Coordf &p1, const Coordf &p2, ColorP color, uint8_t flags) {
		triangles[layer].emplace_back(p0, p1, p2, color, triangle_type_current, flags, lod_current);
		auto idx = triangles[layer].size()-1;
		for(auto &ref: object_refs_current) {
			auto &idxs = object_refs[ref][layer];
			if(idxs.size()) {
				auto &last = idxs.back();
				if(last.second == idx-1) {
					last.second = idx;
				}
				else {
					idxs.emplace_back(idx, idx);
				}
			}
			else {
				idxs.emplace_back(idx, idx);
			}
		}
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

	int Canvas::get_overlay_layer(int layer) {
		if(overlay_layers.count(layer)==0) {
			auto ol = overlay_layer_current++;
			overlay_layers[layer] = ol;
			layer_display[ol].color = Color(1,1,1);
			layer_display[ol].visible = true;
			layer_display[ol].mode = LayerDisplay::Mode::OUTLINE;
		}

		return overlay_layers.at(layer);
	}
}

