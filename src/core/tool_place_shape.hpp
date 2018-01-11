#pragma once
#include "common/shape.hpp"
#include "core.hpp"
#include <forward_list>

namespace horizon {

	class ToolPlaceShape: public ToolBase {
		public :
			ToolPlaceShape (Core *c, ToolID tid);
			ToolResponse begin(const ToolArgs &args) override ;
			ToolResponse update(const ToolArgs &args) override;
			bool can_begin() override;

		protected:
			Shape *temp = 0;
			std::forward_list<Shape*> shapes_placed;


			void create_shape(const Coordi &c);
	};
}
