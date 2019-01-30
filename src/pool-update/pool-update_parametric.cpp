#include "pool-update.hpp"
#include "pool/pool_parametric.hpp"
#include "pool/pool.hpp"
#include "pool/part.hpp"

namespace horizon {

static void status_cb_nop(PoolUpdateStatus st, const std::string msg, const std::string filename)
{
}

void pool_update_parametric(const std::string &pool_base_path, pool_update_cb_t status_cb)
{
    if (!status_cb)
        status_cb = &status_cb_nop;
    PoolParametric pool_parametric(pool_base_path, false);
    status_cb(PoolUpdateStatus::INFO, "", "parametric schema");
    for (const auto &it : pool_parametric.get_tables()) {
        {
            SQLite::Query q(pool_parametric.db, "DROP TABLE IF EXISTS " + it.first);
            q.step();
        }

        std::string qs = "CREATE TABLE '" + it.first + "' (";
        qs += "'uuid' TEXT NOT NULL UNIQUE,";
        for (const auto &it_col : it.second.columns) {

            qs += "'" + it_col.name + "' ";
            switch (it_col.type) {
            case PoolParametric::Column::Type::QUANTITY:
                qs += "DOUBLE";
                break;
            case PoolParametric::Column::Type::ENUM:
                qs += "TEXT";
                break;
            default:
                throw std::runtime_error("unhandled column type");
            }
            qs += ",";
        }
        qs += "PRIMARY KEY('uuid'))";
        std::cout << qs << std::endl;
        SQLite::Query q(pool_parametric.db, qs);
        q.step();
    }

    Pool pool(pool_base_path);
    SQLite::Query q_pool(pool.db, "SELECT uuid FROM parts WHERE parametric_table != ''");
    auto &tables = pool_parametric.get_tables();
    pool_parametric.db.execute("BEGIN TRANSACTION");
    while (q_pool.step()) {
        UUID uu(q_pool.get<std::string>(0));
        auto filename = pool.get_filename(ObjectType::PART, uu);
        auto part = Part::new_from_file(filename, pool);
        status_cb(PoolUpdateStatus::FILE, filename, "");
        auto tab = part.parametric.at("table");
        bool skip = false;
        if (tables.count(tab)) {
            auto &table = tables.at(tab);
            std::string qs = "INSERT INTO " + table.name + " VALUES (?, ";
            for (size_t i = 0; i < table.columns.size(); i++) {
                qs += "?,";
            }
            qs.pop_back();
            qs += ")";
            SQLite::Query q(pool_parametric.db, qs);
            q.bind(1, part.uuid);
            for (size_t i = 0; i < table.columns.size(); i++) {
                auto &col = table.columns.at(i);
                std::string v;
                if (part.parametric.count(col.name)) {
                    v = part.parametric.at(col.name);
                    switch (col.type) {
                    case PoolParametric::Column::Type::QUANTITY: {
                        double d;
                        std::istringstream istr(v);
                        istr.imbue(std::locale("C"));
                        istr >> d;
                        if (!istr.eof() || istr.fail()) {
                            skip = true;
                            status_cb(PoolUpdateStatus::FILE_ERROR, filename,
                                      col.display_name + " '" + v + "' not convertible to double");
                        }
                    } break;

                    case PoolParametric::Column::Type::ENUM: {
                        if (std::find(col.enum_items.begin(), col.enum_items.end(), v) == col.enum_items.end()) {
                            skip = true;
                            status_cb(PoolUpdateStatus::FILE_ERROR, filename,
                                      col.display_name + " value '" + v + "' not in enum");
                        }

                    } break;
                    default:;
                    }
                }
                q.bind(2 + i, v);
            }
            if (!skip)
                q.step();
        }
    }
    pool_parametric.db.execute("COMMIT");
}
} // namespace horizon
