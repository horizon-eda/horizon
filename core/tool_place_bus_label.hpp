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
			void create_attached();
			void delete_attached();
			bool begin_attached();
			bool update_attached(const ToolArgs &args);
			bool check_line(LineNet *li);
			BusLabel *la= nullptr;
			std::forward_list<BusLabel*> labels_placed;
			Bus* bus = nullptr;

	};
}
