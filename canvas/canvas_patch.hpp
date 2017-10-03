#pragma once
#include "canvas/canvas.hpp"
#include "clipper/clipper.hpp"


namespace horizon {
	class CanvasPatch: public Canvas {
		public :
		class PatchKey{
			public:
			PatchType type;
			int layer;
			UUID net;
			bool operator< (const PatchKey &other) const {
				if(type < other.type)
					return true;
				else if(type > other.type)
					return false;

				if(layer < other.layer)
					return true;
				else if(layer > other.layer)
					return false;

				return net < other.net;
			}
		};
		std::map<PatchKey, ClipperLib::Paths> patches;

		CanvasPatch();
		void push() override {}
		void request_push() override;

		private :

			const Net *net = nullptr;
			PatchType patch_type = PatchType::OTHER;
			virtual void img_net(const Net *net) override;
			virtual void img_polygon(const Polygon &poly, bool tr) override;
			virtual void img_hole(const class Hole &hole) override;
			virtual void img_patch_type(PatchType type) override;

	};
}
