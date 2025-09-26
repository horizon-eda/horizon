#pragma once
#include <nlohmann/json_fwd.hpp>
#include "common.hpp"
#include "util/uuid.hpp"

namespace horizon {
using json = nlohmann::json;

class GridSettings {
public:
    GridSettings(const json &j);
    GridSettings()
    {
    }
    json serialize() const;

    class Grid {
    public:
        Grid(const UUID &uu, const json &j);
        Grid(const UUID &uu);
        Grid()
        {
        }
        json serialize() const;
        void assign(const Grid &other);

        UUID uuid;
        std::string name;

        enum class Mode { SQUARE, RECTANGULAR };
        Mode mode = Mode::SQUARE;
        Coordi spacing_rect = Coordi(1.0_mm, 1.0_mm);
        uint64_t spacing_square = 1.0_mm;
        Coordi origin;
    };

    Grid current;
    std::map<UUID, Grid> grids;
};
} // namespace horizon
