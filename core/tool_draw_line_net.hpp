#pragma once
#include "core.hpp"

namespace horizon {
	
	class ToolDrawLineNet : public ToolBase {
		public :
			ToolDrawLineNet(Core *c, ToolID tid);
			ToolResponse begin(const ToolArgs &args) override;
			ToolResponse update(const ToolArgs &args) override;
			bool can_begin() override;
			
		private:
			Junction *temp_junc_head = 0;
			Junction *temp_junc_mid = 0;
			LineNet *temp_line_head = 0;
			LineNet *temp_line_mid = 0;
			enum class BendMode{XY, YX, ARB};
			BendMode bend_mode=BendMode::XY;
			void move_temp_junc(const Coordi &c);
			int merge_nets(Net *net, Net *into);
			ToolResponse end();
			void update_tip();

			Junction *make_temp_junc(const Coordi &c);
	};
}
