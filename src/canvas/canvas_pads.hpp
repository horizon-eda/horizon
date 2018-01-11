#pragma once
#include "canvas.hpp"
#include "clipper/clipper.hpp"
#include "util/uuid.hpp"


namespace horizon {
	class CanvasPads: public Canvas {
		public :
		class PadKey{
			public:
			int layer;
			UUID package;
			UUID pad;
			bool operator< (const PadKey &other) const {
				if(layer < other.layer)
					return true;
				else if(layer > other.layer)
					return false;

				if(package < other.package)
					return true;
				if(package > other.package)
					return false;

				return pad < other.pad;
			}
		};
		std::map<PadKey, std::pair<Placement, ClipperLib::Paths>> pads;

		CanvasPads();
		void push() override {}
		void request_push() override;

		private :
			void img_polygon(const class Polygon &poly, bool tr) override;
	};
}
