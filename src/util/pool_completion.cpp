#include "pool_completion.hpp"
#include "pool/pool.hpp"

namespace horizon {
class CompletionColumns : public Gtk::TreeModel::ColumnRecord {
public:
    CompletionColumns()
    {
        add(manufacturer);
    }

    Gtk::TreeModelColumn<Glib::ustring> manufacturer;
};

CompletionColumns cpl_columns;

Glib::RefPtr<Gtk::EntryCompletion> create_pool_manufacturer_completion(class Pool *pool)
{
    auto cpl = Gtk::EntryCompletion::create();
    auto store = Gtk::ListStore::create(cpl_columns);
    cpl->set_model(store);
    cpl->set_text_column(cpl_columns.manufacturer);

    SQLite::Query q(pool->db,
                    "SELECT DISTINCT manufacturer FROM parts WHERE manufacturer != '' UNION SELECT DISTINCT "
                    "manufacturer FROm packages WHERE manufacturer != '' ORDER BY manufacturer");
    while (q.step()) {
        Gtk::TreeModel::Row row = *(store->append());
        row[cpl_columns.manufacturer] = q.get<std::string>(0);
    }
    return cpl;
}
} // namespace horizon
