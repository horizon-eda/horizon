#pragma once
#include "core.hpp"
#include <deque>
#include <memory>

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

			~ToolRouteTrackInteractive();

		private:
			PNS::ROUTER *router = nullptr;
			PNS::PNS_HORIZON_IFACE *iface = nullptr;
			class CanvasGL *canvas = nullptr;
			ToolWrapper *wrapper = nullptr;

			enum class State {START, ROUTING};
			State state = State::START;

			Board *board = nullptr;
			class BoardRules *rules;
			bool shove = false;

			void update_tip();
	};

}
