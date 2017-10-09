#pragma once
#include "core.hpp"
#include <forward_list>

namespace horizon {

	class ToolUpdateAllPlanes : public ToolBase {
		public :
		ToolUpdateAllPlanes(Core *c, ToolID tid);
			ToolResponse begin(const ToolArgs &args) override;
			ToolResponse update(const ToolArgs &args) override;
			bool can_begin() override;

		private:

	};
}
