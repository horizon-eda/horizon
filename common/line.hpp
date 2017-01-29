#pragma once
#include "uuid.hpp"
#include "json_fwd.hpp"
#include "common.hpp"
#include "junction.hpp"
#include "object.hpp"
#include "position_provider.hpp"
#include "uuid_ptr.hpp"
#include <vector>
#include <map>
#include <fstream>
		

namespace horizon {
	using json = nlohmann::json;
	
	
	class Line {
		public :
			Line(const UUID &uu, const json &j, Object &obj);
			Line(UUID uu);

			UUID uuid;
			uuid_ptr<PositionProvider> to;
			uuid_ptr<PositionProvider> from;
			uint64_t width = 0;
			int layer = 0;
			json serialize() const;
	};
}
