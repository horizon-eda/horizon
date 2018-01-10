#pragma once
#include "canvas/canvas.hpp"

namespace horizon {
	class CanvasGerber: public Canvas {
		public :
		CanvasGerber(class GerberExporter *exp);
		void push() override {}
		void request_push() override;
		uint64_t outline_width = 0;
		private :

			void img_net(const Net *net) override;
			void img_polygon(const Polygon &poly, bool tr) override;
			void img_line(const Coordi &p0, const Coordi &p1, const uint64_t width, int layer, bool tr=true) override;
			void img_padstack(const Padstack &ps) override;
			void img_hole(const Hole &hole) override;
			void img_set_padstack(bool v) override;
			bool padstack_mode = false;

			GerberExporter *exporter;
	};
}
