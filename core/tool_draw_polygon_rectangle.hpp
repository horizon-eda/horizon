#pragma once
#include "core.hpp"

namespace horizon {

	class ToolDrawPolygonRectangle : public ToolBase {
		public :
			ToolDrawPolygonRectangle(Core *c, ToolID tid);
			ToolResponse begin(const ToolArgs &args) override;
			ToolResponse update(const ToolArgs &args) override;
			bool can_begin() override;

		private:
			enum class Mode {CENTER, CORNER};
			enum class Decoration {NONE, CHAMFER, NOTCH};

			Mode mode = Mode::CENTER;
			Decoration decoration = Decoration::NONE;
			int decoration_pos = 0;
			Coordi first_pos;
			Coordi second_pos;
			int step = 0;
			uint64_t decoration_size = 1_mm;

			class Polygon *temp = nullptr;

			void update_polygon();
			void update_tip();

	};
}
