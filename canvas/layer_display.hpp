#pragma once
#include "common.hpp"

namespace horizon {
	class LayerDisplay {
		public:
			enum class Mode {OUTLINE, HATCH, FILL};
			LayerDisplay(bool vi, Mode mo, const Color &c):
				visible(vi), mode(mo), color(c){}
			LayerDisplay() {}

			bool visible=true;
			Mode mode=Mode::FILL;
			Color color;
	};

	class LayerDisplayGL {
		public:
		LayerDisplayGL(const LayerDisplay &ld);
		LayerDisplayGL();
		float r=1;
		float g=1;
		float b=1;
		uint32_t flags=1;
	};
}
