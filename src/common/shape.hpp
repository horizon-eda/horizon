#pragma once
#include "util/uuid.hpp"
#include "common.hpp"
#include "util/uuid_provider.hpp"
#include "json.hpp"
#include "util/placement.hpp"
#include "polygon.hpp"
#include <vector>
#include <map>
#include <fstream>


namespace horizon {
	using json = nlohmann::json;

	/**
	 * For commonly used Pad shapes
	 */
	class Shape: public UUIDProvider {
		public :
			Shape(const UUID &uu, const json &j);
			Shape(const UUID &uu);

			UUID uuid;
			Placement placement;
			int layer = 0;
			std::string parameter_class;

			enum class Form {CIRCLE, RECTANGLE, OBROUND};
			Form form = Form::CIRCLE;
			std::vector<int64_t> params;

			Polygon to_polygon() const;
			std::pair<Coordi, Coordi> get_bbox() const;

			virtual UUID get_uuid() const ;

			json serialize() const;
	};
}
