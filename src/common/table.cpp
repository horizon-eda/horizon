#include "table.hpp"

#include "nlohmann/json.hpp"
#include "util/text_renderer.hpp"
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
    layout_needs_update = true;
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
    layout_needs_update = true;
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

    layout_needs_update = true;
}

std::pair<Coordi, Coordi> Table::get_bbox() const
{
    return {{0, 0}, {total_width, -total_height}};
}

const std::vector<float> &Table::get_row_heights() const
{
    return row_heights;
}

const std::vector<float> &Table::get_col_widths() const
{
    return col_widths;
}

const std::vector<float> &Table::get_baseline_shifts() const
{
    return baseline_shifts;
}

void Table::set_text_size(int64_t size)
{
    this->text_size = size;
    layout_needs_update = true;
}

void Table::set_line_width(int64_t width)
{
    this->line_width = width;
    layout_needs_update = true;
}

void Table::set_padding(int64_t padding)
{
    this->padding = padding;
    layout_needs_update = true;
}

void Table::set_font(TextData::Font font)
{
    this->font = font;
    layout_needs_update = true;
}

void Table::assign_from(const Table &rhs)
{
    n_rows = rhs.n_rows;
    n_columns = rhs.n_columns;
    text_size = rhs.text_size;
    line_width = rhs.line_width;
    padding = rhs.padding;
    font = rhs.font;
    cells = rhs.cells;
}

void Table::update_layout()
{
    if (!layout_needs_update)
        return;

    TextRenderer tr;
    TextRenderer::Options opts;
    opts.draw = false;
    opts.width = line_width;
    opts.font = font;

    auto measure_text = [this, &opts, &tr](const std::string &text) {
        auto [bottom_left, top_right] =
                tr.draw({0, 0}, text_size, text, 0, TextOrigin::BASELINE, ColorP::FROM_LAYER, 0, opts);
        auto extra_space = static_cast<float>(2 * padding + line_width);
        auto text_width = top_right.x - bottom_left.x + extra_space;
        auto text_height = top_right.y - bottom_left.y + extra_space;
        auto baseline_shift = bottom_left.y;
        return std::make_tuple(text_width, text_height, baseline_shift);
    };

    row_heights = std::vector<float>(n_rows);
    col_widths = std::vector<float>(n_columns);
    baseline_shifts = std::vector<float>(n_rows);

    total_height = 0;
    total_width = 0;

    for (int r = 0; r < n_rows; ++r) {
        for (int c = 0; c < n_columns; ++c) {
            auto text = get_cell(r, c);
            auto [width, height, baseline_shift] = measure_text(text);
            if (width > col_widths[c])
                col_widths[c] = width;
            if (height > row_heights[r])
                row_heights[r] = height;
            if (baseline_shift < baseline_shifts[r])
                baseline_shifts[r] = baseline_shift;
        }
    }

    for (int r = 0; r < n_rows; ++r) {
        total_height += static_cast<int64_t>(row_heights[r]);
    }

    for (int c = 0; c < n_columns; ++c) {
        total_width += static_cast<int64_t>(col_widths[c]);
    }

    layout_needs_update = false;
}

} // namespace horizon