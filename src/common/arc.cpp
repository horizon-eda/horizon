#include "common/object_provider.hpp"
#include "common/arc.hpp"
#include "common/lut.hpp"
#include <algorithm>

namespace horizon {
	
	Arc::Arc(const UUID &uu, const json &j, ObjectProvider &obj):
		uuid(uu),
		to(obj.get_junction(j["to"].get<std::string>())),
		from(obj.get_junction(j["from"].get<std::string>())),
		center(obj.get_junction(j["center"].get<std::string>())),
		width(j.value("width", 0)),
		layer(j.value("layer", 0))
	{
	}
	
	Arc::Arc(UUID uu): uuid(uu) {}

	void Arc::reverse() {
		std::swap(to, from);
	}

	json Arc::serialize() const {
			json j;
			j["from"] = (std::string)from.uuid;
			j["to"] = (std::string)to.uuid;
			j["center"] = (std::string)center.uuid;
			j["width"] = width;
			j["layer"] = layer;
			return j;
		}
}
