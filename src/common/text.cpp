#include "text.hpp"
#include "lut.hpp"
#include "nlohmann/json.hpp"

namespace horizon {
static const LutEnumStr<TextOrigin> origin_lut = {
        {"baseline", TextOrigin::BASELINE},
        {"center", TextOrigin::CENTER},
        {"bottom", TextOrigin::BOTTOM},
};

static const LutEnumStr<TextData::Font> font_lut = {
        {"simplex", TextData::Font::SIMPLEX},
        {"complex", TextData::Font::COMPLEX},
        {"complex_italic", TextData::Font::COMPLEX_ITALIC},
        {"complex_small", TextData::Font::COMPLEX_SMALL},
        {"complex_small_italic", TextData::Font::COMPLEX_SMALL_ITALIC},
        {"duplex", TextData::Font::DUPLEX},
        {"triplex", TextData::Font::TRIPLEX},
        {"triplex_italic", TextData::Font::TRIPLEX_ITALIC},
        {"small", TextData::Font::SMALL},
        {"small_italic", TextData::Font::SMALL_ITALIC},
        {"script_simplex", TextData::Font::SCRIPT_SIMPLEX},
        {"script_complex", TextData::Font::SCRIPT_COMPLEX},
};

Text::Text(const UUID &uu, const json &j)
    : uuid(uu), origin(origin_lut.lookup(j.at("origin"))),
      font(font_lut.lookup(j.value("font", ""), TextData::Font::SIMPLEX)), placement(j.at("placement")),
      text(j.at("text").get<std::string>()), size(j.value("size", 2500000)), width(j.value("width", 0)),
      layer(j.value("layer", 0)), from_smash(j.value("from_smash", false))
{
}

Text::Text(const UUID &uu) : uuid(uu)
{
}

json Text::serialize() const
{
    json j;
    j["origin"] = origin_lut.lookup_reverse(origin);
    j["font"] = font_lut.lookup_reverse(font);
    j["text"] = text;
    j["size"] = size;
    j["width"] = width;
    j["layer"] = layer;
    j["from_smash"] = from_smash;
    j["placement"] = placement.serialize();
    return j;
}

UUID Text::get_uuid() const
{
    return uuid;
}
} // namespace horizon
