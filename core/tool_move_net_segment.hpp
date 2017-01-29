#pragma once
#include "core.hpp"

namespace horizon {

	class ToolMoveNetSegment : public ToolBase {
		public :
			ToolMoveNetSegment(Core *c, ToolID tid);
			ToolResponse begin(const ToolArgs &args) override;
			ToolResponse update(const ToolArgs &args) override;
			bool can_begin() override;

		private:
			UUID net_segment;
			UUID get_net_segment();

	};
}
