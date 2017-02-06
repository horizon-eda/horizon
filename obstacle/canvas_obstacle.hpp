#pragma once
#include "canvas/canvas.hpp"
#include "clipper/clipper.hpp"


namespace horizon {
	class CanvasObstacle: public Canvas {
		public :
		CanvasObstacle();
		void push() override {}
		void request_push() override;
		const Net *routing_net = nullptr;
		int routing_layer = 0;
		uint64_t routing_width = 0;
		ClipperLib::Clipper clipper;

		private :

			const Net *net = nullptr;
			virtual void img_net(const Net *net) override;
			virtual void img_polygon(const Polygon &poly) override;

	};
}
