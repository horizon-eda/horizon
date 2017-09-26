#pragma once
#include "core.hpp"
#include "tool_helper_move.hpp"
#include "tool_helper_merge.hpp"
#include <gtkmm.h>

namespace horizon {

	class ToolPaste : public ToolHelperMove, public ToolHelperMerge {
		public :
		ToolPaste(Core *c, ToolID tid);
			virtual ToolResponse begin(const ToolArgs &args);
			virtual ToolResponse update(const ToolArgs &args);

		private:
			void fix_layer(int &la);
			void apply_shift(Coordi &c, const Coordi &cursor_pos);
			Coordi shift;
			bool shift_set = false;

	};
}
