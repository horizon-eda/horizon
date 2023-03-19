#pragma once
#include "common/common.hpp"
#include "common/lut.hpp"
#include "nlohmann/json_fwd.hpp"
#include "util/uuid.hpp"
#include "util/layer_range.hpp"

namespace horizon {
using json = nlohmann::json;

class GerberOutputSettings {
public:
    class GerberLayer {
    public:
        GerberLayer(int l);
        GerberLayer(int l, const json &j);
        json serialize() const;

        int layer;
        std::string filename;
        bool enabled = true;
    };

    GerberOutputSettings(const json &);
    GerberOutputSettings()
    {
    }
    json serialize() const;
    void update_for_board(const class Board &brd);

    std::map<int, GerberLayer> layers;
    std::map<LayerRange, std::string> blind_buried_drills_filenames;

    void update_blind_buried_drills_filenames(const Board &brd);

    enum class DrillMode { INDIVIDUAL, MERGED };
    DrillMode drill_mode = DrillMode::MERGED;

    static const LutEnumStr<DrillMode> mode_lut;

    std::string drill_pth_filename = ".txt";
    std::string drill_npth_filename = "-npth.txt";
    uint64_t outline_width = 0.01_mm;

    std::string prefix;
    std::string output_directory;
    bool zip_output = false;
};
} // namespace horizon
