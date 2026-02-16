#pragma once

#include "util/placement.hpp"
#include "util/text_data.hpp"
#include "util/uuid.hpp"

namespace horizon {

class Table {
public:
    struct Layout {
        int64_t total_width = 0.0;
        int64_t total_height = 0.0;

        std::vector<float> row_heights;
        std::vector<float> col_widths;
        std::vector<Coordf> text_positions;
    };

    Table(const UUID &uu, const nlohmann::json &j);
    Table(const UUID &uu);

    UUID uuid;
    Placement placement;
    int layer = 0;

    std::string get_cell(int row, int col) const;
    std::string get_cell(int index) const;
    void set_cell(int row, int col, const std::string &text);

    UUID get_uuid() const;
    json serialize() const;
    void resize(int rows, int cols);

    int get_n_rows() const
    {
        return n_rows;
    }

    int get_n_columns() const
    {
        return n_columns;
    }

    int64_t get_text_size() const
    {
        return text_size;
    }

    int64_t get_line_width() const
    {
        return line_width;
    }

    int64_t get_padding() const
    {
        return padding;
    }

    TextData::Font get_font() const
    {
        return font;
    }

    void set_text_size(int64_t size);
    void set_line_width(int64_t width);
    void set_padding(int64_t padding);
    void set_font(TextData::Font font);

    void assign_from(const Table &rhs);

    const Layout& get_layout() const;

private:
    mutable std::optional<Layout> cached_layout;

    int n_rows = 2;
    int n_columns = 2;
    int64_t text_size = 1.5_mm;
    int64_t line_width = 0_mm;
    int64_t padding = 0.5_mm;
    TextData::Font font = TextData::Font::SIMPLEX;
    std::vector<std::string> cells;

    Layout compute_layout() const;
};
} // namespace horizon