#pragma once
#include "core.hpp"
#include "tool_place_junction.hpp"
#include "schematic/net_label.hpp"
#include <forward_list>

namespace horizon {

	class ToolPlaceNetLabel : public ToolPlaceJunction {
		public :
			ToolPlaceNetLabel(Core *c, ToolID tid);
			bool can_begin() override;

		protected:
			std::forward_list<NetLabel*> labels_placed;
			void create_attached() override;
			void delete_attached() override;
			bool begin_attached() override;
			bool update_attached(const ToolArgs &args) override;
			bool check_line(LineNet *li) override;
			NetLabel *la = nullptr;
			Orientation last_orientation = Orientation::RIGHT;

	};
}
