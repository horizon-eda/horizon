#pragma once
#include "core.hpp"

namespace horizon {

	class ToolImportDXF : public ToolBase {
		public :
			ToolImportDXF(Core *c, ToolID tid);
			ToolResponse begin(const ToolArgs &args);
			ToolResponse update(const ToolArgs &args);
			bool can_begin();

		private:
	};
}
