#pragma once
#include "common/common.hpp"

namespace horizon {
	class LayerDisplay {
		public:
			enum class Mode {OUTLINE, HATCH, FILL, FILL_ONLY, N_MODES};
			LayerDisplay(bool vi, Mode mo, const Color &c):
				visible(vi), mode(mo), color(c){}
			LayerDisplay() {}

			bool visible=true;
			Mode mode=Mode::FILL;
			Color color;
			uint32_t types_visible = 0xffffffff; //bit mask of Triangle::Type
			uint32_t types_force_outline = 0; //bit mask of Triangle::Type
	};
}
