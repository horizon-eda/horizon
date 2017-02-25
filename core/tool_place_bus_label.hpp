#pragma once
#include "core.hpp"
#include "bus.hpp"
#include "tool_place_junction.hpp"
#include "bus_label.hpp"
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
			bool check_line(LineNet *li) override;
			BusLabel *la= nullptr;
			Orientation last_orientation = Orientation::RIGHT;
			std::forward_list<BusLabel*> labels_placed;
			Bus* bus = nullptr;

	};
}
