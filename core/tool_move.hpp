#pragma once
#include "core.hpp"

namespace horizon {
	
	class ToolMove : public ToolBase {
		public :
			ToolMove(Core *c, ToolID tid);
			ToolResponse begin(const ToolArgs &args) override ;
			ToolResponse update(const ToolArgs &args) override;
			bool can_begin() override;

			
		private:
			Coordi last;
			Coordi selection_center;
			void update_selection_center();
			void expand_selection();
			void transform(Coordi &a, const Coordi &center, bool rotate);
			void mirror_or_rotate(const Coordi &center, bool rotate);
			void do_move(const Coordi &delta);
		
	};
}
