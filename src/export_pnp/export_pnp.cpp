#include "export_pnp.hpp"
#include "board/board.hpp"
#include "util/util.hpp"
#include "util/csv_util.hpp"
#include <glibmm/miscutils.h>

namespace horizon {

enum class ExportSide { TOP, BOTTOM, BOTH };

static void export_PnP(const std::string &filename, const std::vector<PnPRow> &pnp, const PnPExportSettings &settings,
                       ExportSide side)
{
    bool write_headers = true;
    const auto &cols = settings.columns;

    std::vector<std::vector<std::string>> out;
    out.reserve(pnp.size());

    if (write_headers) {
        out.emplace_back();
        auto &x = out.back();
        x.reserve(cols.size());
        for (const auto &it : cols) {
            x.push_back(pnp_column_names.at(it));
        }
    }

    for (const auto &row : pnp) {
        if (side == ExportSide::BOTH || (side == ExportSide::TOP && row.side == PnPRow::Side::TOP)
            || (side == ExportSide::BOTTOM && row.side == PnPRow::Side::BOTTOM)) {
            out.emplace_back();
            auto &x = out.back();
            x.reserve(cols.size());
            for (const auto &col : cols) {
                x.push_back(row.get_column(col));
            }
        }
    }

    auto ofs = make_ofstream(filename, std::ios_base::out | std::ios_base::binary);
    if (!ofs.is_open())
        throw std::runtime_error("output file not opened");

    for (const auto &row : out) {
        for (const auto &col : row) {
            if (needs_quote(col))
                ofs << "\"";
            ofs << escape_csv(col);
            if (needs_quote(col))
                ofs << "\"";
            if (&col != &row.back())
                ofs << ",";
        }
        ofs << "\r\n";
    }
}

void export_PnP(const Board &board, const PnPExportSettings &settings)
{
    auto rows = board.get_PnP(settings);
    std::vector<PnPRow> pnp;
    std::transform(rows.begin(), rows.end(), std::back_inserter(pnp), [](const auto &it) { return it.second; });
    std::sort(pnp.begin(), pnp.end(),
              [](const auto &a, const auto &b) { return strcmp_natural(a.refdes, b.refdes) < 0; });
    if (settings.mode == PnPExportSettings::Mode::MERGED) {
        export_PnP(Glib::build_filename(settings.output_directory, settings.filename_merged), pnp, settings,
                   ExportSide::BOTH);
    }
    else if (settings.mode == PnPExportSettings::Mode::INDIVIDUAL) {
        export_PnP(Glib::build_filename(settings.output_directory, settings.filename_top), pnp, settings,
                   ExportSide::TOP);
        export_PnP(Glib::build_filename(settings.output_directory, settings.filename_bottom), pnp, settings,
                   ExportSide::BOTTOM);
    }
}
} // namespace horizon
