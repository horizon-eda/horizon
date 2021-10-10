#include "pdf_export_settings.hpp"
#include "nlohmann/json.hpp"
#include "util/util.hpp"

namespace horizon {

PDFExportSettings::Layer::Layer() : layer(0)
{
}

static const LutEnumStr<PDFExportSettings::Layer::Mode> mode_lut = {
        {"fill", PDFExportSettings::Layer::Mode::FILL},
        {"outline", PDFExportSettings::Layer::Mode::OUTLINE},
};

PDFExportSettings::Layer::Layer(int l, const json &j)
    : layer(l), color(color_from_json(j.at("color"))), mode(mode_lut.lookup(j.at("mode"))),
      enabled(j.at("enabled").get<bool>())
{
}

json PDFExportSettings::Layer::serialize() const
{
    json j;
    j["color"] = color_to_json(color);
    j["mode"] = mode_lut.lookup_reverse(mode);
    j["enabled"] = enabled;
    return j;
}

PDFExportSettings::Layer::Layer(int l, const Color &c, Mode m, bool e) : layer(l), color(c), mode(m), enabled(e)
{
}

PDFExportSettings::PDFExportSettings(const json &j)
    : output_filename(j.at("output_filename").get<std::string>()), min_line_width(j.at("min_line_width")),
      reverse_layers(j.value("reverse_layers", false)), mirror(j.value("mirror", false)),
      set_holes_size(j.value("set_holes_size", false)), holes_diameter(j.value("holes_diameter", 0))
{
    if (j.count("layers")) {
        const json &o = j.at("layers");
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            int k = std::stoi(it.key());
            layers.emplace(std::piecewise_construct, std::forward_as_tuple(k), std::forward_as_tuple(k, it.value()));
        }
    }
}

PDFExportSettings::PDFExportSettings()
{
}

json PDFExportSettings::serialize_schematic() const
{
    json j;
    j["output_filename"] = output_filename;
    j["min_line_width"] = min_line_width;
    return j;
}

json PDFExportSettings::serialize_board() const
{
    json j = serialize_schematic();
    j["layers"] = json::object();
    for (const auto &it : layers) {
        j["layers"][std::to_string(it.first)] = it.second.serialize();
    }
    j["reverse_layers"] = reverse_layers;
    j["mirror"] = mirror;
    j["set_holes_size"] = set_holes_size;
    j["holes_diameter"] = holes_diameter;
    return j;
}

} // namespace horizon
