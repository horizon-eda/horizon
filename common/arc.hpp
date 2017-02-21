#pragma once
#include "uuid.hpp"
#include "json_fwd.hpp"
#include "common.hpp"
#include "junction.hpp"
#include "object.hpp"
#include "uuid_ptr.hpp"
#include <vector>
#include <map>
#include <fstream>
		

namespace horizon {
	using json = nlohmann::json;
	
	/**
	 * Graphical arc. Same as a Line, an Arc is decorative only.
	 * Since the Arc is over-dimensioned by three coordinates,
	 * the renderer may choose not to render it when its coordinates
	 * don't form a correct arc.
	 */
	class Arc {
		public :
			Arc(const UUID &uu, const json &j, Object &obj);
			Arc(UUID uu);
			void reverse();

			UUID uuid;
			uuid_ptr<Junction> to;
			uuid_ptr<Junction> from;
			uuid_ptr<Junction> center;
			uint64_t width = 0;
			int layer = 0;
			json serialize() const;
	};
}
