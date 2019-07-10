#pragma once
#include "nlohmann/json_fwd.hpp"
#include "common.hpp"
#include <vector>

namespace horizon {
using json = nlohmann::json;

class PDFExportSettings {
public:
    PDFExportSettings(const json &);
    PDFExportSettings();
    json serialize_schematic() const;
    json serialize_board() const;

    std::string output_filename;

    uint64_t min_line_width = 0;

    bool reverse_layers = false;
    bool mirror = false;
    bool include_text = true;

    class Layer {
    public:
        Layer(int l, const json &j);
        Layer();
        enum class Mode { FILL, OUTLINE };
        Layer(int layer, const Color &color, Mode mode, bool enabled);
        json serialize() const;

        int layer;
        Color color;

        Mode mode = Mode::FILL;
        bool enabled = true;
    };

    std::map<int, Layer> layers;
};
} // namespace horizon
