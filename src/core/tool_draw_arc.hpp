#pragma once
#include "core.hpp"

namespace horizon {
	
	class ToolDrawArc : public ToolBase {
		public :
			ToolDrawArc(Core *c, ToolID tid);
			ToolResponse begin(const ToolArgs &args) override;
			ToolResponse update(const ToolArgs &args) override;
			bool can_begin() override;
			
		private:
			enum class DrawArcState {FROM, TO, CENTER};
			DrawArcState state;
			Junction *temp_junc = 0;
			Junction *from_junc = 0;
			Junction *to_junc = 0;
			Arc *temp_arc = 0;
			Junction *make_junction(const Coordi &coords);
			void update_tip();
	};
}
