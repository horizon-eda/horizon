#pragma once
#include "gerber_writer.hpp"
#include "excellon_writer.hpp"
#include "util/layer_range.hpp"
#include <memory>
#include <sstream>

namespace horizon {
class GerberExporter {
    friend class CanvasGerber;

public:
    GerberExporter(const class Board &b, const class GerberOutputSettings &s);
    void generate();
    std::string get_log();

private:
    const class Board &brd;
    const class GerberOutputSettings &settings;
    std::map<int, GerberWriter> writers;
    GerberWriter *get_writer_for_layer(int l);
    ExcellonWriter *get_drill_writer(const LayerRange &span, bool pth);
    std::unique_ptr<ExcellonWriter> drill_writer_pth;
    std::unique_ptr<ExcellonWriter> drill_writer_npth;
    std::map<LayerRange, ExcellonWriter> drill_writers_blind_buried;
    std::stringstream log;

    std::vector<ExcellonWriter *> get_drill_writers();

    void generate_zip();
};
} // namespace horizon
