#pragma once
#include "core.hpp"

namespace horizon {
	class ToolHelperMapSymbol : public virtual ToolBase {
		protected:
		class SchematicSymbol *map_symbol(class Component *c, const class Gate *g);

	};
}
