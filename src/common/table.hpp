#pragma once

#include "util/placement.hpp"
#include "util/text_data.hpp"
#include "util/uuid.hpp"

namespace horizon {

class Table {
public:
    Table(const UUID &uu, const nlohmann::json &j);
    Table(const UUID &uu);

    UUID uuid;
    Placement placement;

    int layer = 0;
    int n_rows = 2;
    int n_columns = 2;
    int64_t text_size = 1.5_mm;
    int64_t line_width = 0_mm;
    int64_t padding = 0.5_mm;
    TextData::Font font = TextData::Font::SIMPLEX;
    std::vector<std::string> cells;

    std::string get_cell(int row, int col) const;
    void set_cell(int row, int col, const std::string &text);

    UUID get_uuid() const;
    json serialize() const;
    void resize(int rows, int cols);
};
} // namespace horizon