#include "pool_parametric.hpp"
#include "nlohmann/json.hpp"
#include <glibmm/miscutils.h>
#include <glibmm/fileutils.h>
#include <iomanip>
#include "util/util.hpp"
#include "common/lut.hpp"
#include "pool_manager.hpp"

namespace horizon {

PoolParametric::Column::Column()
{
}

static std::string get_db_filename(const std::string &db_file, bool read_only)
{
    if (Glib::file_test(db_file, Glib::FILE_TEST_IS_REGULAR) || !read_only)
        return db_file;

    else
        return ":memory:";
}

PoolParametric::PoolParametric(const std::string &bp, bool read_only)
    : db(get_db_filename(bp + "/parametric.db", read_only),
         read_only ? SQLITE_OPEN_READONLY : (SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE), 1000),
      base_path(bp)
{
    {
        SQLite::Query q(db, "ATTACH ? AS pool");
        q.bind(1, Glib::build_filename(bp, "pool.db"));
        q.step();
    }
    std::vector<std::string> table_jsons;

    auto pools = PoolManager::get().get_pools();
    if (pools.count(base_path)) {
        auto &this_pool = pools.at(base_path);
        for (const auto &it : this_pool.pools_included) {
            auto other_pool = PoolManager::get().get_by_uuid(it);
            if (other_pool) {
                auto fn = Glib::build_filename(other_pool->base_path, "tables.json");
                if (Glib::file_test(fn, Glib::FILE_TEST_IS_REGULAR)) {
                    table_jsons.push_back(fn);
                }
            }
        }
    }
    table_jsons.push_back(Glib::build_filename(bp, "tables.json"));

    for (auto it = table_jsons.crbegin(); it != table_jsons.crend(); it++) {
        const auto &fn = *it;
        if (Glib::file_test(fn, Glib::FILE_TEST_IS_REGULAR)) {
            auto j = load_json_from_file(fn);
            const json &o = j.at("tables");
            for (auto it2 = o.cbegin(); it2 != o.cend(); ++it2) {
                std::string table_name = it2.key();
                bool table_exists = has_table(table_name);
                if (table_exists || !read_only)
                    tables.emplace(std::piecewise_construct, std::forward_as_tuple(table_name),
                                   std::forward_as_tuple(table_name, it2.value()));
            }
        }
    }
}

bool PoolParametric::has_table(const std::string &table)
{
    SQLite::Query q(db, "SELECT name FROM sqlite_master WHERE type='table' AND name=?");
    q.bind(1, table);
    if (q.step())
        return true;
    else
        return false;
}

PoolParametric::Table::Table(const std::string &n, const json &j)
    : name(n), display_name(j.at("display_name").get<std::string>())
{
    const json &o = j.at("columns");
    for (auto it = o.cbegin(); it != o.cend(); ++it) {
        columns.emplace_back(it.value());
    }
}

static const LutEnumStr<PoolParametric::Column::Type> type_lut = {
        {"quantity", PoolParametric::Column::Type::QUANTITY},
        {"enum", PoolParametric::Column::Type::ENUM},
};

PoolParametric::Column::Column(const json &j)
    : name(j.at("name").get<std::string>()), display_name(j.at("display_name").get<std::string>()),
      type(type_lut.lookup(j.at("type"))), required(j.value("required", true))
{
    if (type == Type::QUANTITY) {
        unit = j.at("unit").get<std::string>();
        digits = j.value("digits", 3);
        use_si = j.at("use_si");
        no_milli = j.value("no_milli", false);
    }
    else if (type == Type::ENUM) {
        enum_items = j.at("items").get<std::vector<std::string>>();
    }
}


static const std::map<int, std::string> prefixes = {
        {-15, "f"}, {-12, "p"}, {-9, "n"}, {-6, "µ"}, {-3, "m"}, {0, ""}, {3, "k"}, {6, "M"}, {9, "G"}, {12, "T"},
};

std::string PoolParametric::Column::format(double v) const
{
    if (type == Type::QUANTITY) {
        int exp = 0;
        bool negative = v < 0;
        v = std::abs(v);
        if (use_si) {
            while (v >= 1e3 && exp <= 12) {
                v /= 1e3;
                exp += 3;
            }
            if (v > 1e-15) {
                while (v < 1 && exp >= -12) {
                    v *= 1e3;
                    exp -= 3;
                }
            }
        }
        if (exp == -3 && no_milli) {
            if (v < 100) {
                v *= 1e3;
                exp -= 3;
            }
            else {
                v /= 1e3;
                exp += 3;
            }
        }

        std::string prefix = prefixes.at(exp);
        std::stringstream stream;
        stream.imbue(get_locale());
        stream << (negative ? "-" : "");
        if (use_si)
            stream << std::fixed;
        if (digits >= 0)
            stream << std::setprecision(digits);
        stream << v << " " << prefix << unit;
        return stream.str();
    }
    return "";
}

std::string PoolParametric::Column::format(const std::string &v) const
{
    if (v.size() == 0)
        return "N/A";
    else if (type == Type::QUANTITY) {
        double d;
        std::istringstream istr(v);
        istr.imbue(std::locale::classic());
        istr >> d;
        return format(d);
    }
    else {
        return v;
    }
}

const std::map<std::string, PoolParametric::Table> &PoolParametric::get_tables() const
{
    return tables;
}

static const PoolParametric::Column col_manufacturer = [] {
    PoolParametric::Column c;
    c.display_name = "Manufacturer";
    c.name = "_manufacturer";
    c.type = PoolParametric::Column::Type::STRING;
    return c;
}();

static const PoolParametric::Column col_package = [] {
    PoolParametric::Column c;
    c.display_name = "Package";
    c.name = "_package";
    c.type = PoolParametric::Column::Type::STRING;
    return c;
}();

static const std::vector<PoolParametric::Column> extra_columns = {col_manufacturer, col_package};

const std::vector<PoolParametric::Column> &PoolParametric::get_extra_columns()
{
    return extra_columns;
}


} // namespace horizon
