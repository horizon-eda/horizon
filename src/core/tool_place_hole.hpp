#pragma once
#include "common/hole.hpp"
#include "core.hpp"
#include <forward_list>

namespace horizon {
	
	class ToolPlaceHole: public ToolBase {
		public :
			ToolPlaceHole (Core *c, ToolID tid);
			ToolResponse begin(const ToolArgs &args) override ;
			ToolResponse update(const ToolArgs &args) override;
			bool can_begin() override;
			
		protected:
			Hole *temp = 0;
			std::forward_list<Hole*> holes_placed;


			void create_hole(const Coordi &c);
	};
}
