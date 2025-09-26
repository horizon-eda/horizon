#pragma once
#include "common/common.hpp"
#include "common/lut.hpp"
#include <nlohmann/json_fwd.hpp>
#include "util/uuid.hpp"

namespace horizon {
using json = nlohmann::json;

class ODBOutputSettings {
public:
    ODBOutputSettings(const json &);
    ODBOutputSettings()
    {
    }
    json serialize() const;
    void update_for_board(const class Board &brd);

    enum class Format { DIRECTORY, TGZ, ZIP };

    Format format = Format::TGZ;

    std::string job_name;

    std::string output_filename;
    std::string output_directory;
};
} // namespace horizon
