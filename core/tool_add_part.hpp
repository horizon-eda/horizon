#pragma once
#include "core.hpp"
#include "tool_helper_map_symbol.hpp"
#include "tool_helper_move.hpp"

namespace horizon {
	class ToolAddPart :  public ToolHelperMapSymbol, public ToolHelperMove {
		public :
			ToolAddPart(Core *c, ToolID tid);
			ToolResponse begin(const ToolArgs &args) override;
			ToolResponse update(const ToolArgs &args) override;
			bool can_begin() override;

		private:
			unsigned int current_gate = 0;
			class SchematicSymbol *sym_current = nullptr;
			std::vector<const class Gate*> gates;
			class Component *comp = nullptr;
			void update_tip();

	};
}
