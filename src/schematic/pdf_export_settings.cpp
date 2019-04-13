#include "pdf_export_settings.hpp"
#include "nlohmann/json.hpp"

namespace horizon {


PDFExportSettings::PDFExportSettings(const json &j)
    : output_filename(j.at("output_filename").get<std::string>()), min_line_width(j.at("min_line_width"))
{
}

PDFExportSettings::PDFExportSettings()
{
}

json PDFExportSettings::serialize() const
{
    json j;
    j["output_filename"] = output_filename;
    j["min_line_width"] = min_line_width;
    return j;
}

} // namespace horizon
