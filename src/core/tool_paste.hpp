#pragma once
#include "core.hpp"
#include "tool_helper_move.hpp"
#include "tool_helper_merge.hpp"
#include <gtkmm.h>

namespace horizon {

	class ToolPaste : public ToolHelperMove, public ToolHelperMerge {
		public :
		ToolPaste(Core *c, ToolID tid);
			ToolResponse begin(const ToolArgs &args) override;
			ToolResponse update(const ToolArgs &args) override;
			bool can_begin() override {return true;}

		private:
			void fix_layer(int &la);
			void apply_shift(Coordi &c, const Coordi &cursor_pos);
			Coordi shift;
			bool shift_set = false;

	};
}
