#pragma once
#include "nlohmann/json_fwd.hpp"
#include <vector>

namespace horizon {
using json = nlohmann::json;

class PDFExportSettings {
public:
    PDFExportSettings(const json &);
    PDFExportSettings();
    json serialize() const;

    std::string output_filename;

    uint64_t min_line_width = 0;
};
} // namespace horizon
