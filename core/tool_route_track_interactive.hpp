#pragma once
#include "core.hpp"
#include <deque>
#include <memory>
#include <set>

namespace PNS {
 class ROUTER;
 class PNS_HORIZON_IFACE;
 class ITEM;
}

namespace horizon {
	class ToolWrapper;
	class Board;
	class ToolRouteTrackInteractive: public ToolBase {
		friend ToolWrapper;
		public :
			ToolRouteTrackInteractive (Core *c, ToolID tid);
			ToolResponse begin(const ToolArgs &args) override ;
			ToolResponse update(const ToolArgs &args) override;
			bool can_begin() override;
			bool is_specific() override;

			~ToolRouteTrackInteractive();

		private:
			PNS::ROUTER *router = nullptr;
			PNS::PNS_HORIZON_IFACE *iface = nullptr;
			class CanvasGL *canvas = nullptr;
			ToolWrapper *wrapper = nullptr;

			enum class State {START, ROUTING};
			State state = State::START;

			Board *board = nullptr;
			class BoardRules *rules = nullptr;
			bool shove = false;

			void update_tip();
			class Track *get_track(const std::set<SelectableRef> &sel);
	};

}
