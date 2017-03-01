#pragma once
#include "core.hpp"
#include "component.hpp"
#include <forward_list>
#include "imp_interface.hpp"

namespace horizon {

	class ToolManageBuses : public ToolBase {
		public :
		ToolManageBuses(Core *c, ToolID tid);
			ToolResponse begin(const ToolArgs &args) override;
			ToolResponse update(const ToolArgs &args) override;
			bool can_begin() override;

		private:

	};
}
