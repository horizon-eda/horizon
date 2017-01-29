#pragma once
#include "core.hpp"
#include "component.hpp"
#include <forward_list>

namespace horizon {

	class ToolAssignPart : public ToolBase {
		public :
		ToolAssignPart(Core *c, ToolID tid);
			ToolResponse begin(const ToolArgs &args) override;
			ToolResponse update(const ToolArgs &args) override;
			bool can_begin() override;

		private:
			class Entity *get_entity();
			Component *comp;

	};
}
