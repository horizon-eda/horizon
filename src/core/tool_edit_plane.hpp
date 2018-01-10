#pragma once
#include "core.hpp"
#include <forward_list>

namespace horizon {
	class ToolEditPlane : public ToolBase {
		public :
		ToolEditPlane(Core *c, ToolID tid);
			ToolResponse begin(const ToolArgs &args) override;
			ToolResponse update(const ToolArgs &args) override;
			bool can_begin() override;
			bool is_specific() override {return true;}

		private:
			class Polygon *get_poly();
	};
}
