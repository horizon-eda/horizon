#include "canvas_patch.hpp"
#include "assert.h"
#include "core/core_board.hpp"

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

		void CanvasPatch::img_polygon(const Polygon &poly, bool tr) {
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


}
