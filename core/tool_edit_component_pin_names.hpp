#pragma once
#include "core.hpp"
#include "component.hpp"
#include <forward_list>

namespace horizon {

	class ToolEditComponentPinNames : public ToolBase {
		public :
		ToolEditComponentPinNames(Core *c, ToolID tid);
			ToolResponse begin(const ToolArgs &args) override;
			ToolResponse update(const ToolArgs &args) override;
			bool can_begin() override;

		private:
			class Component *get_component();

	};
}
