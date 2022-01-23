#include "text.hpp"
#include "lut.hpp"
#include "nlohmann/json.hpp"

namespace horizon {
static const LutEnumStr<TextOrigin> origin_lut = {
        {"baseline", TextOrigin::BASELINE},
        {"center", TextOrigin::CENTER},
        {"bottom", TextOrigin::BOTTOM},
};

Text::Text(const UUID &uu, const json &j)
    : uuid(uu), origin(origin_lut.lookup(j.at("origin"))),
      font(TextData::font_lut.lookup(j.value("font", ""), TextData::Font::SIMPLEX)), placement(j.at("placement")),
      text(j.at("text").get<std::string>()), size(j.value("size", 2500000)), width(j.value("width", 0)),
      layer(j.value("layer", 0)), allow_upside_down(j.value("allow_upside_down", false)),
      from_smash(j.value("from_smash", false))
{
}

Text::Text(const UUID &uu) : uuid(uu)
{
}

json Text::serialize() const
{
    json j;
    j["origin"] = origin_lut.lookup_reverse(origin);
    j["font"] = TextData::font_lut.lookup_reverse(font);
    j["text"] = text;
    j["size"] = size;
    j["width"] = width;
    j["layer"] = layer;
    j["from_smash"] = from_smash;
    j["placement"] = placement.serialize();
    if (allow_upside_down)
        j["allow_upside_down"] = true;
    return j;
}

UUID Text::get_uuid() const
{
    return uuid;
}
} // namespace horizon
