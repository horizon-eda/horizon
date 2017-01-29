#pragma once
#include "core.hpp"
#include <forward_list>

namespace horizon {
	
	class ToolPlaceText: public ToolBase {
		public :
			ToolPlaceText(Core *c, ToolID tid);
			ToolResponse begin(const ToolArgs &args) override ;
			ToolResponse update(const ToolArgs &args) override;
			bool can_begin() override;
			
		private:
			Text *temp = 0;
			std::forward_list<Text*> texts_placed;

		
	};
}
