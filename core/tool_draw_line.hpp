#pragma once
#include "core.hpp"

namespace horizon {
	
	class ToolDrawLine : public ToolBase {
		public :
			ToolDrawLine(Core *c, ToolID tid);
			ToolResponse begin(const ToolArgs &args) override;
			ToolResponse update(const ToolArgs &args) override;
			bool can_begin() override;
			
		private:
			Junction *temp_junc = 0;
			Line *temp_line = 0;
			void update_tip();
		
	};
}
