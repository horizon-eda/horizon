#include "hole.hpp"
#include "lut.hpp"

namespace horizon {
	using json = nlohmann::json;

	static const LutEnumStr<Hole::Shape> shape_lut = {
		{"round",	Hole::Shape::ROUND},
		{"slot", 	Hole::Shape::SLOT},
	};


	Hole::Hole(const UUID &uu, const json &j):
		uuid(uu),
		placement(j.at("placement")),
		diameter(j.at("diameter").get<uint64_t>()),
		length(j.at("length").get<uint64_t>()),
		parameter_class(j.value("parameter_class", "")),
		plated(j.at("plated").get<bool>()),
		shape(shape_lut.lookup(j.value("shape", "round")))
	{
	}

	UUID Hole::get_uuid() const {
		return uuid;
	}

	Hole::Hole(const UUID &uu): uuid(uu) {}

	json Hole::serialize() const {
		json j;
		j["placement"] = placement.serialize();
		j["diameter"] = diameter;
		j["length"] = length;
		j["shape"] = shape_lut.lookup_reverse(shape);
		j["plated"] = plated;
		j["parameter_class"] = parameter_class;
		return j;
	}
}
