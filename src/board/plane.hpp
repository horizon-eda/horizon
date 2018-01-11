#pragma once
#include "common/polygon.hpp"
#include "block/net.hpp"
#include "clipper/clipper.hpp"

namespace horizon {
	using json = nlohmann::json;

	class PlaneSettings {
		public:
			PlaneSettings(const json &j);
			PlaneSettings() {}
			enum class Style {ROUND, SQUARE, MITER};
			uint64_t min_width = 0.2_mm;
			Style style = Style::ROUND;
			uint64_t extra_clearance = 0;
			bool keep_orphans = false;

			enum class ConnectStyle {SOLID, THERMAL};
			ConnectStyle connect_style = ConnectStyle::SOLID;

			uint64_t thermal_gap_width = 0.2_mm;
			uint64_t thermal_spoke_width = 0.2_mm;

			enum class TextStyle {EXPAND, BBOX};
			TextStyle text_style = TextStyle::EXPAND;

			json serialize() const;
	};

	class Plane: public PolygonUsage {
		public:
			class Fragment {
				public:
				bool orphan = false;
				ClipperLib::Paths paths; //first path is outline, others are holes
				bool contains(const Coordi &c) const; //checks if point is in area defined by paths
			};

			Plane(const UUID &uu, const json &j, class Board &brd);
			Plane(const UUID &uu);
			UUID uuid;
			uuid_ptr<Net> net;
			uuid_ptr<Polygon> polygon;
			bool from_rules = true;
			int priority = 0;
			PlaneSettings settings;

			std::deque<Fragment> fragments;

			Type get_type() const;
			UUID get_uuid() const;
			std::string get_name() const;

			json serialize() const;
	};
}
