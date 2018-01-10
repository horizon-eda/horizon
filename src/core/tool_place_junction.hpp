#pragma once
#include "core.hpp"
#include <forward_list>

namespace horizon {
	
	class ToolPlaceJunction : public ToolBase {
		public :
			ToolPlaceJunction(Core *c, ToolID tid);
			ToolResponse begin(const ToolArgs &args) override ;
			ToolResponse update(const ToolArgs &args) override;
			bool can_begin() override;
			
		protected:
			Junction *temp = 0;
			std::forward_list<Junction*> junctions_placed;


			void create_junction(const Coordi &c);
			virtual void create_attached() {}
			virtual void delete_attached() {}
			virtual bool update_attached(const ToolArgs &args) {return false;}
			virtual bool check_line(class LineNet *li) {return true;}
			virtual bool begin_attached() {return true;}

		
	};
}
