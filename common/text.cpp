#include "text.hpp"
#include "lut.hpp"
#include "json.hpp"

namespace horizon {

	static const LutEnumStr<Orientation> orientation_lut = {
		{"up", 		Orientation::UP},
		{"down", 	Orientation::DOWN},
		{"left", 	Orientation::LEFT},
		{"right",	Orientation::RIGHT},
	};
	static const LutEnumStr<TextPlacement> placement_lut = {
		{"baseline",	TextPlacement::BASELINE},
		{"center", 		TextPlacement::CENTER},
		{"bottom", 		TextPlacement::BOTTOM},
	};

	Text::Text(const UUID &uu, const json &j):
		uuid(uu),
		position(j.at("position").get<std::vector<int64_t>>()),
		placement(placement_lut.lookup(j["placement"])),
		orientation(orientation_lut.lookup(j["orientation"])),
		text(j.at("text").get<std::string>()),
		size(j.value("size", 2500000)),
		width(j.value("width", 0)),
		layer(j.value("layer", 0)),
		from_smash(j.value("from_smash", false)),
		temp(false)
	{
	}

	Text::Text(const UUID &uu): uuid(uu) {}

	json Text::serialize() const {
		json j;
		j["position"] = position.as_array();
		j["placement"] = placement_lut.lookup_reverse(placement);
		j["orientation"] = orientation_lut.lookup_reverse(orientation);
		j["text"] = text;
		j["size"] = size;
		j["width"] = width;
		j["layer"] = layer;
		j["from_smash"] = from_smash;
		return j;
	}

	UUID Text::get_uuid() const {
		return uuid;
	}
}
