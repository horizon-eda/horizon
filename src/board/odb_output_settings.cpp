#include "odb_output_settings.hpp"
#include "common/lut.hpp"
#include "nlohmann/json.hpp"

namespace horizon {

const LutEnumStr<ODBOutputSettings::Format> format_lut = {
        {"directory", ODBOutputSettings::Format::DIRECTORY},
        {"zip", ODBOutputSettings::Format::ZIP},
        {"tgz", ODBOutputSettings::Format::TGZ},
};

ODBOutputSettings::ODBOutputSettings(const json &j)
    : format(format_lut.lookup(j.at("format"))), job_name(j.at("job_name").get<std::string>()),
      output_filename(j.at("output_filename").get<std::string>()),
      output_directory(j.at("output_directory").get<std::string>())
{
}

void ODBOutputSettings::update_for_board(const Board &brd)
{
}

json ODBOutputSettings::serialize() const
{
    json j;
    j["format"] = format_lut.lookup_reverse(format);
    j["job_name"] = job_name;
    j["output_filename"] = output_filename;
    j["output_directory"] = output_directory;
    return j;
}
} // namespace horizon
