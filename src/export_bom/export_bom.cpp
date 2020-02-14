#include "export_bom.hpp"
#include "block/block.hpp"
#include "util/util.hpp"
#include "util/csv_util.hpp"

namespace horizon {

void export_BOM(const std::string &filename, const class Block &block, const class BOMExportSettings &settings)
{
    auto rows = block.get_BOM(settings);
    std::vector<BOMRow> bom;
    std::transform(rows.begin(), rows.end(), std::back_inserter(bom), [](const auto &it) { return it.second; });
    std::sort(bom.begin(), bom.end(), [&settings](const auto &a, const auto &b) {
        auto sa = a.get_column(settings.csv_settings.sort_column);
        auto sb = b.get_column(settings.csv_settings.sort_column);
        auto c = strcmp_natural(sa, sb);
        if (settings.csv_settings.order == BOMExportSettings::CSVSettings::Order::ASC)
            return c < 0;
        else
            return c > 0;
    });

    bool write_headers = true;
    const auto &cols = settings.csv_settings.columns;

    std::vector<std::vector<std::string>> out;
    out.reserve(bom.size());


    if (write_headers) {
        out.emplace_back();
        auto &x = out.back();
        x.reserve(cols.size());
        for (const auto &it : cols) {
            x.push_back(bom_column_names.at(it));
        }
    }

    for (const auto &row : bom) {
        out.emplace_back();
        auto &x = out.back();
        x.reserve(cols.size());
        for (const auto &col : cols) {
            x.push_back(row.get_column(col));
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
} // namespace horizon
