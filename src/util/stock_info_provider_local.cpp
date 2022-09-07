#include "stock_info_provider_local.hpp"
#include "stock_info_provider_local_editor.hpp"
#include <thread>
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


class StockInfoProviderLocalWorker {

public:
    StockInfoProviderLocalWorker(const std::string &pool_base_path, Glib::Dispatcher &disp)
        : pool(pool_base_path), stock_mgr(LocalStockManager::get()), dispatcher(disp),
          prefs(PreferencesProvider::get_prefs().localstock)
    {
        auto worker_thread = std::thread(&StockInfoProviderLocalWorker::worker_wrapper, this);
        worker_thread.detach();
    }
    std::list<std::shared_ptr<StockInfoRecord>> get_records();
    void update_parts(const std::list<UUID> &aparts);
    void save_record(std::shared_ptr<StockInfoRecordLocal> record)
    {
        std::unique_lock<std::mutex> lk(records_to_save_mutex);
        records_to_save.emplace_back(record);
    }

    void load_more();
    void exit()
    {
        worker_exit = true;
        {
            std::unique_lock<std::mutex> lk(parts_mutex);
            parts.clear();
            parts_rev++;
        }

        dispatcher_lock.lock();
        cond.notify_all();
    }

    unsigned int get_n_items_from_cache() const
    {
        return n_items_from_cache;
    }

    unsigned int get_n_items_to_fetch() const
    {
        return n_items_to_fetch;
    }

    unsigned int get_n_items_fetched() const
    {
        return n_items_fetched;
    }

private:
    Pool pool;
    class LocalStockManager &stock_mgr;
    void worker();
    void worker_wrapper()
    {
        worker();
        delete this;
    }
    void add_record(const UUID &uu, const json &j);

    bool worker_exit = false;
    std::condition_variable cond;
    std::list<UUID> parts;
    unsigned int parts_rev = 0;
    std::mutex parts_mutex;

    std::mutex records_mutex;
    std::list<std::shared_ptr<StockInfoRecord>> records;
    std::mutex records_to_save_mutex;
    std::list<std::shared_ptr<StockInfoRecordLocal>> records_to_save;

    std::mutex dispatcher_lock;
    Glib::Dispatcher &dispatcher;
    LocalStockPreferences prefs;
    void emit();

    unsigned int n_items_from_cache = 0;
    unsigned int n_items_to_fetch = 0;
    unsigned int n_items_fetched = 0;
};

void StockInfoProviderLocalWorker::emit()
{
    if (dispatcher_lock.try_lock()) {
        dispatcher.emit();
        dispatcher_lock.unlock();
    }
}

void StockInfoProviderLocalWorker::add_record(const UUID &uu, const json &j)
{
    auto record = std::make_shared<StockInfoRecordLocal>();
    record->uuid = uu;
    record->stock = j.at("qty").get<int>();
    record->price = j.at("price").get<double>();
    record->location = j.at("location").get<std::string>();
    const auto t = j.at("last_updated").get<sqlite3_int64>();
    record->last_updated = Glib::DateTime::create_now_local(t);
    record->state = StockInfoRecordLocal::State::FOUND;
    std::unique_lock<std::mutex> lk(records_mutex);
    records.emplace_back(record);
}

void StockInfoProviderLocalWorker::worker()
{
    unsigned int my_parts_rev = 0;
    while (1) {
        std::list<UUID> my_parts;
        {
            std::unique_lock<std::mutex> lk(parts_mutex);
            cond.wait(lk, [this, &my_parts_rev] { return parts_rev != my_parts_rev; });
            my_parts_rev = parts_rev;
            my_parts = parts;
        }
        if (worker_exit) {
            std::cout << "worker bye" << std::endl;
            return;
        }
        std::cout << "got parts " << my_parts.size() << std::endl;
        std::list<UUID> parts_not_in_db;
        bool cancel = false;
        n_items_from_cache = 0;
        for (const auto &it : my_parts) {
            const auto cr = stock_mgr.query(it);
            if (cr.first) {
                add_record(it, cr.second);
                n_items_from_cache++;
            }
            else {
                parts_not_in_db.emplace_back(it);
            }

            if (my_parts_rev != parts_rev) {
                cancel = true;
                break;
            }
            if (worker_exit) {
                std::cout << "worker bye" << std::endl;
                return;
            }
            emit();
        }
        if (cancel)
            continue;

        if (parts_not_in_db.size()) {
            std::unique_lock<std::mutex> lk(records_mutex);

            for (const auto &uu : parts_not_in_db) {
                auto record = std::make_shared<StockInfoRecordLocal>();
                record->uuid = uu;
                record->state = StockInfoRecordLocal::State::NOT_FOUND;
                records.emplace_back(record);
            }
            emit();
        }

        if (records_to_save.size()) {
            std::unique_lock<std::mutex> lk(records_to_save_mutex);
            for (const auto &it : records_to_save) {
                if (const auto p = dynamic_cast<StockInfoRecordLocal *>(it.get())) {
                    stock_mgr.save(*p);
                }
            }
            records_to_save.clear();
            emit();
        }

        emit();
    }
    std::cout << "worker bye" << std::endl;
}

std::list<std::shared_ptr<StockInfoRecord>> StockInfoProviderLocalWorker::get_records()
{
    std::list<std::shared_ptr<StockInfoRecord>> r;
    {
        std::unique_lock<std::mutex> lk(records_mutex);
        r = records;
        records.clear();
    }
    return r;
}

StockInfoProviderLocal::StockInfoProviderLocal(const std::string &pool_base_path)
    : worker(new StockInfoProviderLocalWorker(pool_base_path, dispatcher))
{
    dispatcher.connect([this] {
        if (status_label) {
            std::string txt = format_digits(worker->get_n_items_from_cache(), 5) + " from database";
            if (worker->get_n_items_to_fetch()) {
                txt += ", fetching " + format_m_of_n(worker->get_n_items_fetched(), worker->get_n_items_to_fetch());
            }
            status_label->set_text(txt);
        }
    });
}

void StockInfoProviderLocal::update_parts(const std::list<UUID> &parts)
{
    worker->update_parts(parts);
}

void StockInfoProviderLocal::update_stock_info(std::shared_ptr<StockInfoRecordLocal> record)
{
    worker->save_record(record);
    worker->load_more();
}

std::list<std::shared_ptr<StockInfoRecord>> StockInfoProviderLocal::get_records()
{
    return worker->get_records();
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
                    auto ed = StockInfoProviderLocalEditor::create(*this, p);
                    ed->show();
                }
            }
        }
    }
}

static std::string get_currency_sign(const std::string &s)
{
    static const std::map<std::string, std::string> signs = {
            {"USD", "$"},
            {"EUR", "€"},
            {"EUR", "€"},
            {"GBP", "£"},
    };
    if (signs.count(s))
        return signs.at(s);
    else
        return s + " ";
}

void StockInfoProviderLocalWorker::update_parts(const std::list<UUID> &aparts)
{
    {
        std::unique_lock<std::mutex> lk(parts_mutex);
        parts = aparts;
        parts_rev++;
    }
    cond.notify_all();
}

void StockInfoProviderLocalWorker::load_more()
{
    {
        std::unique_lock<std::mutex> lk(parts_mutex);
        parts_rev++;
    }
    cond.notify_all();
}


StockInfoProviderLocal::~StockInfoProviderLocal()
{
    worker->exit();
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
