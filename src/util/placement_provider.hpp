#pragma once
#include "util/placement.hpp"

namespace horizon {
	class PlacementProvider {
		public:
		virtual Placement get_placement() const = 0;
		virtual ~PlacementProvider() {}
	};
}
