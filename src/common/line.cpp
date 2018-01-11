#include "common/object_provider.hpp"
#include "common/line.hpp"
#include "common/lut.hpp"

namespace horizon {
	
	Line::Line(const UUID &uu, const json &j, ObjectProvider &obj):
		uuid(uu),
		to(obj.get_junction(j.at("to").get<std::string>())),
		from(obj.get_junction(j.at("from").get<std::string>())),
		width(j.value("width", 0)),
		layer(j.value("layer", 0))
	{
	}
	
	Line::Line(UUID uu): uuid(uu) {}

	json Line::serialize() const {
		json j;
		j["from"] = (std::string)from.uuid;
		j["to"] = (std::string)to.uuid;
		j["width"] = width;
		j["layer"] = layer;
		return j;
	}
}
