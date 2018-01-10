#include "cores.hpp"
#include "core.hpp"
#include "core_schematic.hpp"
#include "core_symbol.hpp"
#include "core_padstack.hpp"
#include "core_package.hpp"
#include "core_board.hpp"

namespace horizon {
	Cores::Cores(Core *co): r(co),
			c(dynamic_cast<CoreSchematic*>(r)),
			y(dynamic_cast<CoreSymbol*>(r)),
			a(dynamic_cast<CorePadstack*>(r)),
			k(dynamic_cast<CorePackage*>(r)),
			b(dynamic_cast<CoreBoard*>(r))
			{}
}
