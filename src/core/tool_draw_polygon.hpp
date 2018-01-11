#pragma once
#include "core.hpp"
#include "common/polygon.hpp"

namespace horizon {

	class ToolDrawPolygon : public ToolBase {
		public :
			ToolDrawPolygon(Core *c, ToolID tid);
			ToolResponse begin(const ToolArgs &args) override;
			ToolResponse update(const ToolArgs &args) override;
			bool can_begin() override;

		private:
			Polygon *temp = nullptr;
			Polygon::Vertex *vertex = nullptr;
			Polygon::Vertex *last_vertex = nullptr;
			bool arc_mode = false;
			void update_tip();
	};
}
