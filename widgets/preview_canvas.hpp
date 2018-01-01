#pragma once
#include "canvas/canvas.hpp"

namespace horizon {
	class PreviewCanvas: public CanvasGL {
		public:
			PreviewCanvas(class Pool &pool);
			void load(ObjectType ty, const UUID &uu);

		private:
			class Pool &pool;
	};
}
