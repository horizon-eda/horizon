#pragma once
#include "core.hpp"
#include "tool_place_junction.hpp"
#include "block/bus.hpp"

namespace horizon {

	class ToolPlaceBusRipper : public ToolPlaceJunction {
		public :
			ToolPlaceBusRipper(Core *c, ToolID tid);
			bool can_begin() override;

		private:
			void create_attached() override;
			void delete_attached() override;
			bool begin_attached() override;
			bool update_attached(const ToolArgs &args) override;
			bool check_line(class LineNet *li) override;
			class BusRipper *ri= nullptr;
			Orientation last_orientation = Orientation::RIGHT;
			Bus* bus = nullptr;

			std::vector<Bus::Member*> bus_members;
			size_t bus_member_current = 0;
	};
}
