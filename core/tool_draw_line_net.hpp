#pragma once
#include "core.hpp"
#include "tool_helper_merge.hpp"

namespace horizon {
	
	class ToolDrawLineNet: public ToolHelperMerge {
		public :
			ToolDrawLineNet(Core *c, ToolID tid);
			ToolResponse begin(const ToolArgs &args) override;
			ToolResponse update(const ToolArgs &args) override;
			bool can_begin() override;
			
		private:
			Junction *temp_junc_head = 0;
			Junction *temp_junc_mid = 0;
			class LineNet *temp_line_head = 0;
			class LineNet *temp_line_mid = 0;
			enum class BendMode{XY, YX, ARB};
			BendMode bend_mode=BendMode::XY;
			void move_temp_junc(const Coordi &c);
			ToolResponse end();
			void update_tip();

			Junction *make_temp_junc(const Coordi &c);
	};
}
