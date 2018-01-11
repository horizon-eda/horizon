#include "common/text.hpp"
#include "common/lut.hpp"

namespace horizon {
	static const LutEnumStr<TextOrigin> origin_lut = {
		{"baseline",	TextOrigin::BASELINE},
		{"center", 		TextOrigin::CENTER},
		{"bottom", 		TextOrigin::BOTTOM},
	};

	Text::Text(const UUID &uu, const json &j):
		uuid(uu),
		origin(origin_lut.lookup(j["origin"])),
		placement(j.at("placement")),
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
		j["origin"] = origin_lut.lookup_reverse(origin);
		j["text"] = text;
		j["size"] = size;
		j["width"] = width;
		j["layer"] = layer;
		j["from_smash"] = from_smash;
		j["placement"] = placement.serialize();
		return j;
	}

	UUID Text::get_uuid() const {
		return uuid;
	}
}
