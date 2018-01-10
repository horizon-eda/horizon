#pragma once
#include "core.hpp"
#include "tool_helper_map_symbol.hpp"
#include "tool_helper_move.hpp"

namespace horizon {
	class ToolMapSymbol : public ToolHelperMapSymbol, public ToolHelperMove {
		public :
			ToolMapSymbol(Core *c, ToolID tid);
			ToolResponse begin(const ToolArgs &args) override;
			ToolResponse update(const ToolArgs &args) override;
			bool can_begin() override;
		private:
			void update_tip();
			std::map<UUIDPath<2>, std::string> gates_out;
			class SchematicSymbol *sym_current = nullptr;
	};
}
