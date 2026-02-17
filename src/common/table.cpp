#include "table.hpp"

#include "nlohmann/json.hpp"
#include "util/text_renderer.hpp"
#include "util/uuid.hpp"

#include <iostream>

namespace horizon {

Table::Table(const UUID &uu, const json &j)
    : uuid(uu), placement(j.at("placement")), layer(j.value("layer", 0)), n_rows(j.value("n_rows", 2)),
      n_cols(j.value("n_columns", 2)), text_size(j.value("text_size", 1.5_mm)), line_width(j.value("line_width", 0_mm)),
      padding(j.value("padding", 0.5_mm)), font(TextData::font_lut.lookup(j.value("font", ""), TextData::Font::SIMPLEX))
{
    resize(n_rows, n_cols);
    if (j.contains("cells")) {
        const auto &cells_json = j.at("cells");
        for (size_t i = 0; i < std::min(cells_json.size(), cells.size()); i++) {
            cells[i] = cells_json[i].get<std::string>();
        }
    }
}

Table::Table(const UUID &uu) : uuid(uu)
{
    resize(n_rows, n_cols);
}

std::string Table::get_cell(size_t row, size_t col) const
{
    auto idx = row * n_cols + col;
    if (idx >= cells.size())
        return "?";
    return cells[idx];
}

void Table::set_cell(size_t row, size_t col, const std::string &text)
{
    if (row >= n_rows || col >= n_cols)
        return;
    cells[row * n_cols + col] = text;
    cached_layout.reset();
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
    j["n_columns"] = n_cols;
    j["text_size"] = text_size;
    j["line_width"] = line_width;
    j["padding"] = padding;
    j["font"] = TextData::font_lut.lookup_reverse(font);
    j["cells"] = cells;
    return j;
}

void Table::resize(size_t rows, size_t cols)
{
    std::vector<std::string> new_cells(rows * cols);
    for (size_t r = 0; r < std::min(rows, n_rows); r++) {
        for (size_t c = 0; c < std::min(cols, n_cols); c++) {
            if (r * n_cols + c < cells.size()) {
                new_cells[r * cols + c] = cells[r * n_cols + c];
            }
        }
    }
    cells = std::move(new_cells);

    n_rows = rows;
    n_cols = cols;

    cached_layout.reset();
}

void Table::set_text_size(uint64_t val)
{
    text_size = val;
    cached_layout.reset();
}

void Table::set_line_width(uint64_t val)
{
    line_width = val;
    cached_layout.reset();
}

void Table::set_padding(uint64_t val)
{
    padding = val;
    cached_layout.reset();
}

void Table::set_font(TextData::Font val)
{
    font = val;
    cached_layout.reset();
}

void Table::assign_from(const Table &rhs)
{
    n_rows = rhs.n_rows;
    n_cols = rhs.n_cols;
    text_size = rhs.text_size;
    line_width = rhs.line_width;
    padding = rhs.padding;
    font = rhs.font;
    cells = rhs.cells;
}

Table::Layout Table::compute_layout() const
{
    std::cout << "compute layout" << std::endl;

    const size_t cell_count = n_rows * n_cols;

    TextRenderer tr;
    TextRenderer::Options opts;
    opts.draw = false;
    opts.width = line_width;
    opts.font = font;

    Layout layout;
    layout.row_heights = std::vector<float>(n_rows);
    layout.col_widths = std::vector<float>(n_cols);
    layout.text_positions = std::vector<Coordf>(cell_count);

    auto baseline_shifts = std::vector<float>(n_rows);
    auto left_bearings = std::vector<float>(cell_count);

    const auto extra_space = static_cast<float>(2 * padding + line_width);

    size_t cell_idx = 0;
    for (size_t r = 0; r < n_rows; ++r) {
        float y_max = 0;
        for (size_t c = 0; c < n_cols; ++c) {
            auto [bottom_left, top_right] =
                    tr.draw({0, 0}, text_size, get_cell(r, c), 0, TextOrigin::BASELINE, ColorP::FROM_LAYER, 0, opts);

            if (bottom_left.y < baseline_shifts[r]) {
                baseline_shifts[r] = bottom_left.y;
            }
            if (top_right.y > y_max) {
                y_max = top_right.y;
            }

            auto col_width = top_right.x - bottom_left.x + extra_space;
            if (col_width > layout.col_widths[c]) {
                layout.col_widths[c] = col_width;
            }

            left_bearings[cell_idx++] = bottom_left.x;
        }
        layout.row_heights[r] = y_max - baseline_shifts[r] + extra_space;
    }

    for (size_t r = 0; r < n_rows; ++r) {
        layout.total_height += layout.row_heights[r];
    }
    for (size_t c = 0; c < n_cols; ++c) {
        layout.total_width += layout.col_widths[c];
    }

    auto y = static_cast<float>(padding);
    cell_idx = 0;
    for (size_t r = 0; r < n_rows; r++) {
        y -= layout.row_heights[r];
        auto x = static_cast<float>(padding);
        for (size_t c = 0; c < n_cols; c++) {
            auto pos = Coordf{x + line_width / 2 - left_bearings[cell_idx], y + line_width / 2 - baseline_shifts[r]};
            layout.text_positions[cell_idx++] = pos;
            x += layout.col_widths[c];
        }
    }

    return layout;
}

const Table::Layout &Table::get_layout() const
{
    if (!cached_layout.has_value()) {
        cached_layout = compute_layout();
    }
    return *cached_layout;
}

bool Table::is_empty() const
{
    return std::all_of(cells.begin(), cells.end(), [](const auto &cell) { return cell.empty(); });
}

} // namespace horizon