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
			Junction *temp_junc = 0;
			Junction *temp_junc_2 = 0;
			LineNet *temp_line = 0;
			LineNet *temp_line_2 = 0;
			enum class Restrict{NONE, X, Y};
			Restrict restrict_mode=Restrict::NONE;
			void move_temp_junc(const Coordi &c);
			int merge_nets(Net *net, Net *into);
			ToolResponse end(bool delete_junction=true);
	};
}
