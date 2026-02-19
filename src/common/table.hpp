#pragma once

#include "util/placement.hpp"
#include "util/text_data.hpp"
#include "util/uuid.hpp"

namespace horizon {

class Table {
public:
    struct Layout {
        float total_width = 0.0;
        float total_height = 0.0;

        std::vector<float> row_heights;
        std::vector<float> col_widths;
        std::vector<Coordf> text_positions;
    };

    Table(const UUID &uu, const nlohmann::json &j);
    Table(const UUID &uu);

    UUID uuid;
    Placement placement;
    int layer = 0;

    std::string get_cell(size_t row, size_t col) const;
    void set_cell(size_t row, size_t col, const std::string &text);

    UUID get_uuid() const;
    json serialize() const;
    void resize(size_t rows, size_t cols);

    size_t get_n_rows() const
    {
        return n_rows;
    }

    size_t get_n_columns() const
    {
        return n_cols;
    }

    uint64_t get_text_size() const
    {
        return text_size;
    }

    uint64_t get_line_width() const
    {
        return line_width;
    }

    uint64_t get_padding() const
    {
        return padding;
    }

    TextData::Font get_font() const
    {
        return font;
    }

    void set_text_size(uint64_t val);
    void set_line_width(uint64_t val);
    void set_padding(uint64_t val);
    void set_font(TextData::Font val);

    void assign_from(const Table &rhs);

    const Layout &get_layout() const;

    const std::vector<std::string> &get_cells() const
    {
        return cells;
    }

    bool is_empty() const;

private:
    mutable std::optional<Layout> cached_layout;

    size_t n_rows = 2;
    size_t n_cols = 2;
    uint64_t text_size = 1.5_mm;
    uint64_t line_width = 0_mm;
    uint64_t padding = 0.5_mm;
    TextData::Font font = TextData::Font::SIMPLEX;
    std::vector<std::string> cells;

    Layout compute_layout() const;
};
} // namespace horizon