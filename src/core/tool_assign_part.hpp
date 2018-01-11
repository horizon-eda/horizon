#pragma once
#include "core.hpp"
#include "block/component.hpp"
#include <forward_list>

namespace horizon {

	class ToolAssignPart : public ToolBase {
		public :
		ToolAssignPart(Core *c, ToolID tid);
			ToolResponse begin(const ToolArgs &args) override;
			ToolResponse update(const ToolArgs &args) override;
			bool can_begin() override;
			bool is_specific() override {return true;}

		private:
			const class Entity *get_entity();
			Component *comp;

	};
}
