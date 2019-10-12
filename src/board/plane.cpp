#include "plane.hpp"
#include "board.hpp"
#include "common/lut.hpp"
#include "nlohmann/json.hpp"

namespace horizon {

static const LutEnumStr<PlaneSettings::Style> style_lut = {
        {"square", PlaneSettings::Style::SQUARE},
        {"miter", PlaneSettings::Style::MITER},
        {"round", PlaneSettings::Style::ROUND},
};

static const LutEnumStr<PlaneSettings::ConnectStyle> connect_style_lut = {
        {"solid", PlaneSettings::ConnectStyle::SOLID},
        {"thermal", PlaneSettings::ConnectStyle::THERMAL},
};

static const LutEnumStr<PlaneSettings::TextStyle> text_style_lut = {
        {"expand", PlaneSettings::TextStyle::EXPAND},
        {"bbox", PlaneSettings::TextStyle::BBOX},
};

static const LutEnumStr<PlaneSettings::FillStyle> fill_style_lut = {
        {"solid", PlaneSettings::FillStyle::SOLID},
        {"hatch", PlaneSettings::FillStyle::HATCH},
};

PlaneSettings::PlaneSettings(const json &j)
    : min_width(j.at("min_width")), keep_orphans(j.at("keep_orphans")),
      thermal_gap_width(j.value("thermal_gap_width", 0.1_mm)),
      thermal_spoke_width(j.value("thermal_spoke_width", 0.2_mm)),
      hatch_border_width(j.value("hatch_border_width", 0.5_mm)), hatch_line_width(j.value("hatch_line_width", 0.2_mm)),
      hatch_line_spacing(j.value("hatch_line_spacing", 0.5_mm))
{
    if (j.count("style")) {
        style = style_lut.lookup(j.at("style"));
    }
    if (j.count("connect_style")) {
        connect_style = connect_style_lut.lookup(j.at("connect_style"));
    }
    if (j.count("text_style")) {
        text_style = text_style_lut.lookup(j.at("text_style"));
    }
    if (j.count("fill_style")) {
        fill_style = fill_style_lut.lookup(j.at("fill_style"));
    }
}

json PlaneSettings::serialize() const
{
    json j;
    j["min_width"] = min_width;
    j["keep_orphans"] = keep_orphans;
    j["style"] = style_lut.lookup_reverse(style);
    j["connect_style"] = connect_style_lut.lookup_reverse(connect_style);
    j["thermal_gap_width"] = thermal_gap_width;
    j["thermal_spoke_width"] = thermal_spoke_width;
    j["text_style"] = text_style_lut.lookup_reverse(text_style);
    j["fill_style"] = fill_style_lut.lookup_reverse(fill_style);
    j["hatch_border_width"] = hatch_border_width;
    j["hatch_line_spacing"] = hatch_line_spacing;
    j["hatch_line_width"] = hatch_line_width;
    return j;
}

Plane::Plane(const UUID &uu, const json &j, Board &brd)
    : uuid(uu), net(&brd.block->nets.at(j.at("net").get<std::string>())),
      polygon(&brd.polygons.at(j.at("polygon").get<std::string>())), from_rules(j.value("from_rules", true)),
      priority(j.value("priority", 0))
{
    if (j.count("settings")) {
        settings = PlaneSettings(j.at("settings"));
    }
    if (j.count("fragments")) {
        for (const auto &it : j.at("fragments")) {
            fragments.emplace_back(it);
        }
    }
}

Plane::Plane(const UUID &uu) : uuid(uu)
{
}

PolygonUsage::Type Plane::get_type() const
{
    return PolygonUsage::Type::PLANE;
}

UUID Plane::get_uuid() const
{
    return uuid;
}

json Plane::serialize() const
{
    json j;
    j["net"] = (std::string)net->uuid;
    j["polygon"] = (std::string)polygon->uuid;
    j["priority"] = priority;
    j["settings"] = settings.serialize();
    auto frags = json::array();
    for (const auto &it : fragments) {
        frags.push_back(it.serialize());
    }
    j["fragments"] = frags;
    return j;
}

bool Plane::Fragment::contains(const Coordi &c) const
{
    ClipperLib::IntPoint pt(c.x, c.y);
    if (ClipperLib::PointInPolygon(pt, paths.front()) != 0) {    // point is within or on contour
        for (size_t i = 1; i < paths.size(); i++) {              // check all holes
            if (ClipperLib::PointInPolygon(pt, paths[i]) == 1) { // point is in hole
                return false;
            }
        }
        return true;
    }
    else { // point is outside of contour
        return false;
    }
}

Plane::Fragment::Fragment(const json &j) : orphan(j.at("orphan"))
{
    const auto &j_paths = j.at("paths");
    paths.reserve(j_paths.size());
    for (const auto &j_path : j_paths) {
        paths.emplace_back();
        auto &path = paths.back();
        path.reserve(j_path.size());
        for (const auto &it : j_path) {
            path.emplace_back(it.at(0), it.at(1));
        }
    }
}

json Plane::Fragment::serialize() const
{
    json j;
    j["orphan"] = orphan;
    auto j_paths = json::array();
    for (const auto &path : paths) {
        auto j_path = json::array();
        for (const auto &it : path) {
            j_path.push_back({it.X, it.Y});
        }
        j_paths.push_back(j_path);
    }
    j["paths"] = j_paths;
    return j;
}

} // namespace horizon
