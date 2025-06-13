#include "dimension.hpp"
#include "lut.hpp"
#include <nlohmann/json.hpp>

namespace horizon {

static const LutEnumStr<Dimension::Mode> mode_lut = {
        {"distance", Dimension::Mode::DISTANCE},
        {"horizontal", Dimension::Mode::HORIZONTAL},
        {"vertical", Dimension::Mode::VERTICAL},
};

Dimension::Dimension(const UUID &uu) : uuid(uu)
{
}

Dimension::Dimension(const UUID &uu, const json &j)
    : uuid(uu), p0(j.at("p0").get<std::vector<int64_t>>()), p1(j.at("p1").get<std::vector<int64_t>>()),
      label_distance(j.at("label_distance")), label_size(j.value("label_size", 1.5_mm)),
      mode(mode_lut.lookup(j.at("mode")))
{
}

json Dimension::serialize() const
{
    json j;
    j["p0"] = p0.as_array();
    j["p1"] = p1.as_array();
    j["label_distance"] = label_distance;
    j["label_size"] = label_size;
    j["mode"] = mode_lut.lookup_reverse(mode);
    return j;
}

int64_t Dimension::project(const Coordi &c) const
{
    Coordi v;
    switch (mode) {
    case Mode::DISTANCE:
        v = p1 - p0;
        break;

    case Mode::HORIZONTAL:
        v = {std::abs(p1.x - p0.x), 0};
        break;

    case Mode::VERTICAL:
        v = {0, -std::abs(p1.y - p0.y)};
        break;
    }
    Coordi w = Coordi(-v.y, v.x);
    return w.dot(c) / w.magd();
}

int64_t Dimension::get_length() const
{
    switch (mode) {
    case Mode::DISTANCE:
        return (p0 - p1).magd();
    case Mode::HORIZONTAL:
        return std::abs(p0.x - p1.x);
    case Mode::VERTICAL:
        return std::abs(p0.y - p1.y);
    default:
        return 0;
    }
}
} // namespace horizon
