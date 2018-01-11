#include "canvas_patch.hpp"
#include "common/hole.hpp"
#include "board/plane.hpp"

namespace horizon {
	CanvasPatch::CanvasPatch() : Canvas::Canvas() {
		img_mode = true;
	}
	void CanvasPatch::request_push() {

	}

	void CanvasPatch::img_net(const Net *n) {
		net = n;
	}

	void CanvasPatch::img_patch_type(PatchType pt) {
		patch_type = pt;
	}

	void CanvasPatch::img_hole(const Hole &hole) {
		//create patch of type HOLE_PTH/HOLE_NPTH on layer 10000
		//for NPTH, set net to 0
		auto net_saved = net;
		auto patch_type_saved = patch_type;
		if(!hole.plated) {
			net = nullptr;
			patch_type = PatchType::HOLE_NPTH;
		}
		else {
			patch_type = PatchType::HOLE_PTH;
		}
		auto poly = hole.to_polygon().remove_arcs(64);
		img_polygon(poly, true);
		net = net_saved;
		patch_type = patch_type_saved;
	}

	void CanvasPatch::img_polygon(const Polygon &poly, bool tr) {
		if(poly.usage == nullptr) { //regular patch
			PatchKey patch_key{patch_type, poly.layer, net?net->uuid:UUID()};
			patches[patch_key];

			patches[patch_key].emplace_back();

			ClipperLib::Path &t = patches[patch_key].back();
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
		else if(auto plane = dynamic_cast<Plane*>(poly.usage.ptr)){
			PatchKey patch_key{PatchType::PLANE, poly.layer, plane->net?plane->net->uuid:UUID()};
			patches[patch_key];

			for(const auto &frag: plane->fragments) {
				patches[patch_key].emplace_back();
				ClipperLib::Path &contour = patches[patch_key].back();
				contour = frag.paths.front();

				bool needs_reverse = false;
				if(ClipperLib::Orientation(contour)) {
					std::reverse(contour.begin(), contour.end());
					needs_reverse = true;
				}

				for(size_t i=1; i<frag.paths.size(); i++) {
					patches[patch_key].emplace_back();
					ClipperLib::Path &hole = patches[patch_key].back();
					hole = frag.paths[i];
					if(needs_reverse) {
						std::reverse(hole.begin(), hole.end());
					}
				}
			}
		}
	}

	void CanvasPatch::img_text(const Text &txt, std::pair<Coordf, Coordf> &extents) {
		std::pair<Coordi, Coordi> ext(Coordi(extents.first.x, extents.first.y), Coordi(extents.second.x, extents.second.y));
		text_extents.emplace(txt.layer, ext.first, ext.second);
	}
}
