#pragma once
#include "util/uuid.hpp"
#include "json.hpp"
#include "common/common.hpp"
#include "common/junction.hpp"
#include "util/uuid_ptr.hpp"
#include <vector>
#include <map>
#include <fstream>

namespace horizon {
	using json = nlohmann::json;
	
	/**
	 * Graphical line. A Line is purely decorative, it doesn't
	 * imply electrical connection. For drawing closed regions, use
	 * Polygon.
	 */
	class Line {
		public :
			Line(const UUID &uu, const json &j, class ObjectProvider &obj);
			Line(UUID uu);

			UUID uuid;
			uuid_ptr<Junction> to;
			uuid_ptr<Junction> from;
			uint64_t width = 0;
			int layer = 0;
			json serialize() const;
	};
}
