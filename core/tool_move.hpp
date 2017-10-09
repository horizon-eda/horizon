#pragma once
#include "core.hpp"
#include "tool_helper_move.hpp"
#include "tool_helper_merge.hpp"

namespace horizon {
	
	class ToolMove : public ToolHelperMove, public ToolHelperMerge {
		public :
			ToolMove(Core *c, ToolID tid);
			ToolResponse begin(const ToolArgs &args) override ;
			ToolResponse update(const ToolArgs &args) override;
			bool can_begin() override;
			bool is_specific() override {return true;}
			
		private:
			Coordi last;
			Coordi origin;
			Coordi selection_center;
			void update_selection_center();
			void expand_selection();
			void update_tip();
			enum class Mode {X, Y, ARB};
			Mode mode = Mode::ARB;
			Coordi get_coord(const Coordi &c);
			void do_move(const Coordi &c);
		
	};
}
