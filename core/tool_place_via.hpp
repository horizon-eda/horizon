#pragma once
#include "core.hpp"
#include "tool_place_junction.hpp"
#include <forward_list>

namespace horizon {

	class ToolPlaceVia: public ToolPlaceJunction {
		public :
			ToolPlaceVia(Core *c, ToolID tid);
			bool can_begin() override;

		protected:
			void create_attached() override;
			void delete_attached() override;
			bool begin_attached() override;
			bool update_attached(const ToolArgs &args) override;
			class Via *via = nullptr;
			const class Padstack *padstack = nullptr;

			std::forward_list<class Via*> vias_placed;

		private:

	};
}
