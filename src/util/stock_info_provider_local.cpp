#include "stock_info_provider_local.hpp"
#include "stock_info_provider_local_editor.hpp"
#include "nlohmann/json.hpp"
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


class LocalStockManager {
public:
    static LocalStockManager &get()
    {
        static LocalStockManager inst;
        return inst;
    }

    std::pair<bool, json> query(const UUID &part)
    {
        SQLite::Query q(stock_db, "SELECT qty, price, location, last_updated FROM stock_info WHERE uuid=?");
        q.bind(1, part);
        if (q.step()) {
            json j;
            j["qty"] = q.get<int>(0);
            j["price"] = q.get<double>(1);
            j["location"] = q.get<std::string>(2);
            j["last_updated"] = q.get<sqlite3_int64>(3);
            return {true, j};
        }
        else {
            return {false, nullptr};
        }
    }

    bool save(const StockInfoRecordLocal &record)
    {
        SQLite::Query q(stock_db,
                        "INSERT OR REPLACE INTO stock_info "
                        "(uuid, qty, price, location, last_updated) VALUES(?, ?, ?, ?, ?) ");
        q.bind(1, record.uuid);
        q.bind(2, record.stock);
        q.bind(3, record.price);
        q.bind(4, record.location);
        q.bind_int64(5, Glib::DateTime::create_now_local().to_unix());
        return q.step();
    }

private:
    LocalStockManager()
        : prefs(PreferencesProvider::get_prefs().localstock),
          stock_db(prefs.database_path, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, 10000)
    {
        int user_version = stock_db.get_user_version();
        if (user_version < min_user_version) {
            // update schema
            auto bytes = Gio::Resource::lookup_data_global(
                    "/org/horizon-eda/horizon/util/"
                    "stock_info_provider_local_schema.sql");
            gsize size{bytes->get_size() + 1}; // null byte
            auto data = (const char *)bytes->get_data(size);
            stock_db.execute(data);
        }
    }
    LocalStockPreferences prefs;
    SQLite::Database stock_db;
};

StockInfoRecordLocal::StockInfoRecordLocal(const UUID &uu) : uuid(uu), last_updated(Glib::DateTime::create_now_local(0))
{
}

bool StockInfoRecordLocal::load()
{
    const auto cr = LocalStockManager::get().query(uuid);
    if (cr.first) {
        auto j = cr.second;
        stock = j.at("qty").get<int>();
        price = j.at("price").get<double>();
        location = j.at("location").get<std::string>();
        if (const auto t = j.at("last_updated").get<sqlite3_int64>())
            last_updated = Glib::DateTime::create_now_local(t);
        state = StockInfoRecordLocal::State::FOUND;
    }
    else {
        state = StockInfoRecordLocal::State::NOT_FOUND;
    }
    return cr.first;
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
