#pragma once
#include "core.hpp"
#include "board/track.hpp"

namespace horizon {

	class ToolDrawTrack: public ToolBase {
		public :
			ToolDrawTrack(Core *c, ToolID tid);
			ToolResponse begin(const ToolArgs &args) override;
			ToolResponse update(const ToolArgs &args) override;
			bool can_begin() override;

		private:
			Junction *temp_junc = 0;
			Track *temp_track = 0;
			Track *create_temp_track();
			class BoardRules *rules;
	};
}
