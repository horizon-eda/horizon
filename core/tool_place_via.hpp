#pragma once
#include "core.hpp"
#include "tool_place_junction.hpp"
#include "via.hpp"
#include <forward_list>

namespace horizon {

	class ToolPlaceVia: public ToolPlaceJunction {
		public :
			ToolPlaceVia(Core *c, ToolID tid);
			bool can_begin() override;

		protected:
			void create_attached();
			void delete_attached();
			bool begin_attached();
			bool update_attached(const ToolArgs &args);
			Via *via = nullptr;
			Padstack *padstack = nullptr;
			std::forward_list<Via*> vias_placed;

		private:

	};
}
