#include "plane.hpp"
#include "board.hpp"
#include "lut.hpp"


namespace horizon {

	static const LutEnumStr<PlaneSettings::Style> style_lut = {
		{"square",	PlaneSettings::Style::SQUARE},
		{"miter",	PlaneSettings::Style::MITER},
		{"round",	PlaneSettings::Style::ROUND},
	};


	PlaneSettings::PlaneSettings(const json &j): min_width(j.at("min_width")),
		keep_orphans(j.at("keep_orphans"))
	{
		if(j.count("style")) {
			style = style_lut.lookup(j.at("style"));
		}
	}

	json PlaneSettings::serialize() const {
		json j;
		j["min_width"] = min_width;
		j["keep_orphans"] = keep_orphans;
		j["style"] = style_lut.lookup_reverse(style);
		return j;
	}

	Plane::Plane(const UUID &uu, const json &j, Board &brd): uuid(uu),
		net(&brd.block->nets.at(j.at("net").get<std::string>())),
		polygon(&brd.polygons.at(j.at("polygon").get<std::string>())),
		from_rules(j.value("from_rules", true)),
		priority(j.value("priority", 0))
	{
		if(j.count("settings")) {
			settings = PlaneSettings(j.at("settings"));
		}
	}

	Plane::Plane(const UUID& uu): uuid(uu) {}

	PolygonUsage::Type Plane::get_type() const {
		return PolygonUsage::Type::PLANE;
	}

	UUID Plane::get_uuid() const {
		return uuid;
	}

	json Plane::serialize() const {
		json j;
		j["net"] = (std::string)net->uuid;
		j["polygon"] = (std::string)polygon->uuid;
		j["priority"] = priority;
		j["settings"] = settings.serialize();
		return j;
	}

	bool Plane::Fragment::contains(const Coordi &c) const {
		ClipperLib::IntPoint pt(c.x, c.y);
		if(ClipperLib::PointInPolygon(pt, paths.front())==1) { //point is within contour
			for(size_t i=1; i<paths.size(); i++) { //check all holes
				if(ClipperLib::PointInPolygon(pt, paths[i]) == 1) { //point is in hole
					return false;
				}
			}
			return true;
		}
		else { //point is outside of contour
			return false;
		}
	}
}
