#include "canvas_obstacle.hpp"
#include "assert.h"
#include "core/core_board.hpp"

namespace horizon {
		CanvasObstacle::CanvasObstacle() : Canvas::Canvas() {
			img_mode = true;
		}
		void CanvasObstacle::request_push() {}

		void CanvasObstacle::img_net(const Net *n) {
			net = n;
		}

		void CanvasObstacle::img_polygon(const Polygon &poly, bool tr) {
			if(net == routing_net)
				return;
			if(!net)
				return;
			if(poly.layer != routing_layer)
				return;

			ClipperLib::Path t;
			t.reserve(poly.vertices.size());
			for(const auto &it: poly.vertices) {
				auto p = transform.transform(it.position);
				t.emplace_back(p.x, p.y);
				//std::cout << it.position.x << " " << it.position.y << std::endl;
			}
			if(ClipperLib::Orientation(t)) {
				std::reverse(t.begin(), t.end());
			}
			ClipperLib::ClipperOffset ofs;
			ofs.ArcTolerance = 10e3;
			ofs.AddPath(t, ClipperLib::jtRound, ClipperLib::etClosedPolygon);

			ClipperLib::Paths t_ofs;
			Cores cores(core);
			auto clearance = &cores.b->get_net_classes()->default_clearance;
			if(net && routing_net)
				clearance = cores.b->get_net_classes()->get_clearance(net->net_class, routing_net->net_class);
			uint64_t expand = clearance->routing_clearance;
			ofs.Execute(t_ofs, expand+routing_width/2);
			assert(t_ofs.size()==1);


			clipper.AddPath(t_ofs[0], ClipperLib::ptSubject, true);

		}


}
