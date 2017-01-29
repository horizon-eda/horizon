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
}
