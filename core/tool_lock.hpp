#pragma once
#include "core.hpp"
#include <set>

namespace horizon {

	class ToolLock : public ToolBase {
		public :
			ToolLock(Core *c, ToolID tid);
			ToolResponse begin(const ToolArgs &args) override;
			ToolResponse update(const ToolArgs &args) override;
			bool can_begin() override;
			bool is_specific() override;

		private:
			std::set<class Track*> get_tracks(bool locked);
			std::set<class Via*> get_vias(bool locked);

	};
}
