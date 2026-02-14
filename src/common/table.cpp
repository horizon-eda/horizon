#include "table.hpp"

#include "nlohmann/json.hpp"
#include "util/uuid.hpp"

namespace horizon {

Table::Table(const UUID &uu, const json &j)
    : uuid(uu), placement(j.at("placement")), layer(j.value("layer", 0)), n_rows(j.value("n_rows", 2)),
      n_columns(j.value("n_columns", 2)), text_size(j.value("text_size", 1.5_mm)),
      line_width(j.value("line_width", 0_mm)), padding(j.value("padding", 0.5_mm)),
      font(TextData::font_lut.lookup(j.value("font", ""), TextData::Font::SIMPLEX))
{
    resize(n_rows, n_columns);
    if (j.contains("cells")) {
        const auto &cells_json = j.at("cells");
        for (size_t i = 0; i < std::min(cells_json.size(), cells.size()); i++) {
            cells[i] = cells_json[i].get<std::string>();
        }
    }
}

Table::Table(const UUID &uu) : uuid(uu)
{
    resize(n_rows, n_columns);
}

std::string Table::get_cell(int row, int col) const
{
    if (row < 0 || row >= n_rows || col < 0 || col >= n_columns)
        return "";
    return cells[row * n_columns + col];
}

void Table::set_cell(int row, int col, const std::string &text)
{
    if (row < 0 || row >= n_rows || col < 0 || col >= n_columns)
        return;
    cells[row * n_columns + col] = text;
}

UUID Table::get_uuid() const
{
    return uuid;
}

json Table::serialize() const
{
    json j;
    j["placement"] = placement.serialize();
    j["layer"] = layer;
    j["n_rows"] = n_rows;
    j["n_columns"] = n_columns;
    j["text_size"] = text_size;
    j["line_width"] = line_width;
    j["padding"] = padding;
    j["font"] = TextData::font_lut.lookup_reverse(font);
    j["cells"] = cells;
    return j;
}

void Table::resize(int rows, int cols)
{
    std::vector<std::string> new_cells(rows * cols);
    for (unsigned int r = 0; r < std::min(rows, n_rows); r++) {
        for (unsigned int c = 0; c < std::min(cols, n_columns); c++) {
            if (r * n_columns + c < cells.size()) {
                new_cells[r * cols + c] = cells[r * n_columns + c];
            }
        }
    }
    cells = std::move(new_cells);
    n_rows = rows;
    n_columns = cols;
}

} // namespace horizon