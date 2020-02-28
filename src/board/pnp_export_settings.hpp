#pragma once
#include "common/lut.hpp"
#include "util/uuid.hpp"
#include "nlohmann/json_fwd.hpp"
#include "pnp.hpp"
#include <vector>

namespace horizon {
using json = nlohmann::json;

class PnPExportSettings {
public:
    PnPExportSettings(const json &j);
    PnPExportSettings();
    json serialize() const;

    enum class Format { CSV, TEXT };
    Format format = Format::CSV;

    std::vector<PnPColumn> columns;

    enum class Mode { INDIVIDUAL, MERGED };
    Mode mode = Mode::MERGED;

    static const LutEnumStr<Mode> mode_lut;

    bool include_nopopulate = true;

    std::string output_directory;

    std::string filename_top;
    std::string filename_bottom;
    std::string filename_merged;
};
} // namespace horizon
