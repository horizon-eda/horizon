#pragma once
#include "core.hpp"
#include <forward_list>

namespace horizon {

	class ToolBendLineNet : public ToolBase {
		public :
			ToolBendLineNet(Core *c, ToolID tid);
			ToolResponse begin(const ToolArgs &args) override;
			ToolResponse update(const ToolArgs &args) override;
			bool can_begin() override;
			bool is_specific() override {return true;}

		private:
			Junction *temp = 0;
			std::forward_list<Junction*> junctions_placed;

	};
}
