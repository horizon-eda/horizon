#pragma once
#include "uuid.hpp"
#include "json.hpp"
#include "common.hpp"
#include "uuid_ptr.hpp"
#include "junction.hpp"
#include "bus.hpp"
#include <vector>
#include <map>
#include <set>
#include <fstream>


namespace horizon {
	using json = nlohmann::json;

	/**
	 * Makes a Bus available on the schematic. Contrary to NetLabel, a BusLabel
	 * forces its Bus on the attached Junction.
	 */
	class BusLabel {
		public :
			BusLabel(const UUID &uu, const json &j, class Sheet &sheet, class Block &block);
			BusLabel(const UUID &uu);

			const UUID uuid;

			enum class Style {PLAIN, FLAG};
			Style style = Style::FLAG;

			uuid_ptr<Junction> junction;
			Orientation orientation = Orientation::RIGHT;
			uint64_t size = 1.5_mm;
			std::set<unsigned int> on_sheets;
			bool offsheet_refs = true;
			uuid_ptr<Bus> bus;

			json serialize() const;
	};
}
