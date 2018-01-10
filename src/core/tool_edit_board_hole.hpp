#pragma once
#include "core.hpp"

namespace horizon {

	class ToolEditBoardHole : public ToolBase {
		public :
		ToolEditBoardHole(Core *c, ToolID tid);
			ToolResponse begin(const ToolArgs &args) override;
			ToolResponse update(const ToolArgs &args) override;
			bool can_begin() override;
			bool is_specific() override {return true;}

		private:
			std::set<class BoardHole*> get_holes();
	};
}
