#pragma once
#include "core.hpp"

namespace horizon {
	class ToolAddComponent : public ToolBase {
		public :
			ToolAddComponent(Core *c, ToolID tid);
			ToolResponse begin(const ToolArgs &args) override;
			ToolResponse update(const ToolArgs &args) override;
			bool can_begin() override;
	};
}
