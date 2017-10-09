#pragma once
#include "core.hpp"
#include <forward_list>

namespace horizon {

	class ToolEditVia: public ToolBase {
		public :
		ToolEditVia(Core *c, ToolID tid);
			ToolResponse begin(const ToolArgs &args) override;
			ToolResponse update(const ToolArgs &args) override;
			bool can_begin() override;
			bool is_specific() override {return true;}
	};
}
