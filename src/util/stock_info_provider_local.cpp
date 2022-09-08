#include "stock_info_provider_local.hpp"
#include "stock_info_provider_local_editor.hpp"
#include "preferences/preferences_provider.hpp"
#include "util/gtk_util.hpp"
#include "util/util.hpp"
#include "util/win32_undef.hpp"
#include "preferences/preferences.hpp"
#include "util/str_util.hpp"
#include <list>
#include <iomanip>
#include <atomic>
#include <iostream>

namespace horizon {

static const int min_user_version = 1; // keep in sync with schema
static const std::string database_schema = "/org/horizon-eda/horizon/util/stock_info_provider_local_schema.sql";

class LocalStockManager {
public:
    static LocalStockManager &get()
    {
        static LocalStockManager inst;
        return inst;
    }

    bool query(StockInfoRecordLocal &record)
    {
        search_query.reset();
        search_query.bind(1, {record.uuid.get_bytes(), 16});
        if (search_query.step()) {
            record.stock = search_query.get<int>(0);
            record.price = search_query.get<double>(1);
            record.location = search_query.get<std::string>(2);
            record.last_updated = Glib::DateTime::create_now_local(search_query.get<sqlite3_int64>(3));
            return true;
        }
        return false;
    }

    bool save(const StockInfoRecordLocal &record)
    {
        insert_query.reset();
        insert_query.bind(1, {record.uuid.get_bytes(), 16});
        insert_query.bind(2, record.stock);
        insert_query.bind(3, record.price);
        insert_query.bind(4, record.location);
        insert_query.bind_int64(5, Glib::DateTime::create_now_local().to_unix());
        return insert_query.step();
    }

private:
    LocalStockManager()
        : prefs(PreferencesProvider::get_prefs().localstock),
          stock_db(prefs.database_path, database_schema, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, 10000,
                   min_user_version),
          insert_query(stock_db,
                       "INSERT OR REPLACE INTO stock_info "
                       "(uuid, qty, price, location, last_updated) VALUES(?, ?, ?, ?, ?) "),
          search_query(stock_db, "SELECT qty, price, location, last_updated FROM stock_info WHERE uuid=?")
    {
    }
    LocalStockPreferences prefs;
    SQLite::Database stock_db;
    // Reuse prepared statements to supposively speed up queries
    // see https://www.sqlite.org/cintro.htm
    SQLite::Query insert_query;
    SQLite::Query search_query;
};

StockInfoRecordLocal::StockInfoRecordLocal(const UUID &uu) : uuid(uu), last_updated(Glib::DateTime::create_now_local(0))
{
}

bool StockInfoRecordLocal::load()
{
    const auto found = LocalStockManager::get().query(*this);
    if (found) {
        state = StockInfoRecordLocal::State::FOUND;
    }
    else {
        state = StockInfoRecordLocal::State::NOT_FOUND;
    }
    return found;
}

bool StockInfoRecordLocal::save()
{
    return LocalStockManager::get().save(*this);
}

StockInfoProviderLocal::StockInfoProviderLocal(const std::string &pool_base_path) : pool(pool_base_path)
{
    dispatcher.connect([this] {
        if (status_label) {
            std::string txt = format_digits(n_in_stock, 5) + " in local stock";
            status_label->set_text(txt);
        }
    });
}

void StockInfoProviderLocal::update_parts(const std::list<UUID> &parts)
{
    records.clear();
    n_in_stock = 0;
    for (const auto &part : parts) {
        auto record = std::make_shared<StockInfoRecordLocal>(part);
        if (record->load())
            n_in_stock += 1;
        records.push_back(record);
    }
    dispatcher.emit();
}

std::list<std::shared_ptr<StockInfoRecord>> StockInfoProviderLocal::get_records()
{
    return records;
}

void StockInfoProviderLocal::add_columns(Gtk::TreeView *atreeview,
                                         Gtk::TreeModelColumn<std::shared_ptr<StockInfoRecord>> acolumn)
{
    assert(!treeview);
    treeview = atreeview;
    column = acolumn;
    tvc = Gtk::manage(new Gtk::TreeViewColumn("Local Stock"));
    auto cr = Gtk::manage(new Gtk::CellRendererText());
    {
        auto attributes_list = pango_attr_list_new();
        auto attribute_font_features = pango_attr_font_features_new("tnum 1");
        pango_attr_list_insert(attributes_list, attribute_font_features);
        g_object_set(G_OBJECT(cr->gobj()), "attributes", attributes_list, NULL);
        pango_attr_list_unref(attributes_list);
    }
    tvc->set_cell_data_func(*cr, [this](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
        Gtk::TreeModel::Row row = *it;
        auto mcr = dynamic_cast<Gtk::CellRendererText *>(tcr);
        auto v = row.get_value(column);
        if (v == nullptr) {
            mcr->property_text() = "Loading…";
        }
        else {
            if (auto p = dynamic_cast<const StockInfoRecordLocal *>(v.get())) {
                if (p->state == StockInfoRecordLocal::State::FOUND) {
                    std::ostringstream oss;
                    oss.imbue(std::locale(""));
                    oss << p->stock;
                    mcr->property_text() = oss.str();
                }
                else if (p->state == StockInfoRecordLocal::State::NOT_AVAILABLE) {
                    mcr->property_text() = "N/A";
                }
                else if (p->state == StockInfoRecordLocal::State::NOT_LOADED) {
                    mcr->property_text() = "Not loaded";
                }
                else {
                    mcr->property_text() = "Not found";
                }
            }
        }
    });
    tvc->pack_start(*cr, false);

    treeview->append_column(*tvc);

    treeview->signal_button_press_event().connect_notify(sigc::mem_fun(*this, &StockInfoProviderLocal::handle_click));
}

void StockInfoProviderLocal::handle_click(GdkEventButton *ev)
{
    const int RIGHT_CLICK = 3;
    if (ev->button == RIGHT_CLICK) {
        Gdk::Rectangle rect;
        Gtk::TreeModel::Path path;
        Gtk::TreeViewColumn *col;
        int cell_x, cell_y;
        if (treeview->get_path_at_pos(ev->x, ev->y, path, col, cell_x, cell_y)) {
            if (col == tvc) {
                treeview->get_cell_area(path, *col, rect);
                int x, y;
                x = rect.get_x();
                y = rect.get_y();
                treeview->convert_bin_window_to_widget_coords(x, y, x, y);
                rect.set_x(x);
                rect.set_y(y);

                auto v = treeview->get_model()->get_iter(path)->get_value(column);
                if (auto p = std::dynamic_pointer_cast<StockInfoRecordLocal>(v)) {
                    auto ed = StockInfoProviderLocalEditor::create(p);
                    ed->show();
                }
            }
        }
    }
}

StockInfoProviderLocal::~StockInfoProviderLocal()
{
}

Gtk::Widget *StockInfoProviderLocal::create_status_widget()
{
    status_label = Gtk::manage(new Gtk::Label(""));
    status_label->get_style_context()->add_class("dim-label");
    status_label->property_margin() = 2;
    label_set_tnum(status_label);
    return status_label;
}

} // namespace horizon
