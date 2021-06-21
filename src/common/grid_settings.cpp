#include "grid_settings.hpp"
#include "nlohmann/json.hpp"
#include "common/lut.hpp"

namespace horizon {

static const LutEnumStr<GridSettings::Grid::Mode> mode_lut = {
        {"square", GridSettings::Grid::Mode::SQUARE},
        {"rect", GridSettings::Grid::Mode::RECTANGULAR},
};

GridSettings::Grid::Grid(const UUID &uu, const json &j)
    : uuid(uu), name(j.at("name").get<std::string>()), mode(mode_lut.lookup(j.at("mode").get<std::string>())),
      spacing_rect(j.at("spacing_rect").get<std::vector<int64_t>>()),
      spacing_square(j.at("spacing_square").get<int64_t>()), origin(j.at("origin").get<std::vector<int64_t>>())
{
}

GridSettings::Grid::Grid(const UUID &uu) : uuid(uu)
{
}

json GridSettings::Grid::serialize() const
{
    json j;
    j["mode"] = mode_lut.lookup_reverse(mode);
    j["spacing_square"] = spacing_square;
    j["spacing_rect"] = spacing_rect.as_array();
    j["origin"] = origin.as_array();
    j["name"] = name;
    return j;
}

void GridSettings::Grid::assign(const Grid &other)
{
    mode = other.mode;
    spacing_rect = other.spacing_rect;
    spacing_square = other.spacing_square;
    origin = other.origin;
}


GridSettings::GridSettings(const json &j) : current(UUID(), j.at("current"))
{
    for (const auto &[k, v] : j.at("grids").items()) {
        const auto uu = UUID(k);
        grids.emplace(std::piecewise_construct, std::forward_as_tuple(uu), std::forward_as_tuple(uu, v));
    }
}

json GridSettings::serialize() const
{
    json j;
    j["current"] = current.serialize();
    j["grids"] = json::object();
    for (const auto &[uu, grid] : grids) {
        j["grids"][(std::string)uu] = grid.serialize();
    }
    return j;
}


} // namespace horizon
