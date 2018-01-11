#include "canvas_pads.hpp"
#include "common/hole.hpp"
#include "board/plane.hpp"
#include "board/board_layers.hpp"

namespace horizon {
	CanvasPads::CanvasPads() : Canvas::Canvas() {
		img_mode = true;
	}
	void CanvasPads::request_push() {

	}

	void CanvasPads::img_polygon(const Polygon &poly, bool tr) {
		if(!BoardLayers::is_copper(poly.layer))
			return;
		if(object_refs_current.size() >= 2 && object_refs_current.back().type == ObjectType::PAD) {
			auto pad_uuid = object_refs_current.back().uuid;
			auto pkg_uuid = object_refs_current.back().uuid2;
			PadKey pad_key {poly.layer, pkg_uuid, pad_uuid};
			if(pads.count(pad_key)) {
				assert(pads.at(pad_key).first.shift == transform.shift); //basic sanity check
			}
			pads[pad_key].first = transform;
			pads[pad_key].second.emplace_back();
			ClipperLib::Path &t = pads[pad_key].second.back();
			t.reserve(poly.vertices.size());
			if(tr) {
				for(const auto &it: poly.vertices) {
					auto p = transform.transform(it.position);
					t.emplace_back(p.x, p.y);
				}
			}
			else {
				for(const auto &it: poly.vertices) {
					t.emplace_back(it.position.x, it.position.y);
				}
			}
			if(ClipperLib::Orientation(t)) {
				std::reverse(t.begin(), t.end());
			}
		}
	}
}
