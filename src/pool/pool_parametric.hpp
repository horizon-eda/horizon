#pragma once
#include "util/sqlite.hpp"
#include <map>
#include <vector>
#include "nlohmann/json_fwd.hpp"

namespace horizon {
using json = nlohmann::json;

class PoolParametric {
public:
    PoolParametric(const std::string &base_path, bool read_only = true);

    class Column {
    public:
        Column();
        Column(const json &j);
        std::string name;
        std::string display_name;
        enum class Type { QUANTITY, STRING, ENUM };
        Type type = Type::STRING;
        std::string unit;
        bool use_si = true;
        bool no_milli = false;
        int digits = -1;
        std::vector<std::string> enum_items;

        std::string format(const std::string &v) const;
        std::string format(double v) const;
    };

    class Table {
    public:
        Table(const std::string &name, const json &j);
        std::string name;
        std::string display_name;
        std::vector<Column> columns;
    };

    const std::string &get_base_path() const;
    const std::map<std::string, Table> &get_tables() const;
    static const std::vector<Column> &get_extra_columns();

    SQLite::Database db;

private:
    std::string base_path;
    std::map<std::string, Table> tables;
    bool has_table(const std::string &table);
};
} // namespace horizon
