#pragma once
#include "core.hpp"

namespace horizon {
	class ToolDrawDimension : public ToolBase {
		public :
			ToolDrawDimension(Core *c, ToolID tid);
			ToolResponse begin(const ToolArgs &args) override;
			ToolResponse update(const ToolArgs &args) override;
			bool can_begin() override;

		private:
			class Dimension *temp = nullptr;
			void update_tip();

			enum class State {P0, P1, LABEL};
			State state = State::P0;
	};
}
