#pragma once
#include "uuid.hpp"
#include "json_fwd.hpp"
#include "common.hpp"
#include "uuid_ptr.hpp"
#include "junction.hpp"
#include <vector>
#include <map>
#include <set>
#include <fstream>


namespace horizon {
	using json = nlohmann::json;

	/**
	 * Displays the junction's Net name it is attached to. A net
	 * label doesn't 'force' its Net on the attached junction, it merely
	 * tells its Net name.
	 */
	class NetLabel {
		public :
			NetLabel(const UUID &uu, const json &j, class Sheet &sheet);
			NetLabel(const UUID &uu);

			const UUID uuid;

			enum class Style {PLAIN, FLAG};
			Style style = Style::FLAG;

			uuid_ptr<Junction> junction;
			Orientation orientation = Orientation::RIGHT;
			uint64_t size = 1.25_mm;
			std::set<unsigned int> on_sheets;
			bool offsheet_refs = true;

			json serialize() const;
	};
}
