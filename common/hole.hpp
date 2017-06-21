#pragma once
#include "uuid.hpp"
#include "common.hpp"
#include "uuid_provider.hpp"
#include "json.hpp"
#include <vector>
#include <map>
#include <fstream>
#include "placement.hpp"


namespace horizon {
	using json = nlohmann::json;

	/**
	 * A hole with diameter and position, that's it.
	 */
	class Hole: public UUIDProvider {
		public :
			Hole(const UUID &uu, const json &j);
			Hole(const UUID &uu);

			UUID uuid;
			Placement placement;
			uint64_t diameter = 0.5_mm;
			uint64_t length = 0.5_mm;
			std::string parameter_class;

			/**
			 * true if this hole is PTH, false if NPTH.
			 * Used by the gerber exporter.
			 */
			bool plated = false;

			enum class Shape {ROUND, SLOT};
			Shape shape = Shape::ROUND;


			virtual UUID get_uuid() const ;

			//not stored
			json serialize() const;
	};
}
