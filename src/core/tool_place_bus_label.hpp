#pragma once
#include "core.hpp"
#include "tool_place_junction.hpp"
#include <forward_list>

namespace horizon {

	class ToolPlaceBusLabel : public ToolPlaceJunction {
		public :
			ToolPlaceBusLabel(Core *c, ToolID tid);
			bool can_begin() override;

		private:
			void create_attached() override;
			void delete_attached() override;
			bool begin_attached() override;
			bool update_attached(const ToolArgs &args) override;
			bool check_line(class LineNet *li) override;
			class BusLabel *la= nullptr;
			Orientation last_orientation = Orientation::RIGHT;
			std::forward_list<class BusLabel*> labels_placed;
			class Bus* bus = nullptr;

	};
}
