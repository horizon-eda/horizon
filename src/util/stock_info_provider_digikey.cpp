#include "stock_info_provider_digikey.hpp"
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

namespace horizon {

static const int min_user_version = 1; // keep in sync with schema

std::string StockInfoProviderDigiKey::get_db_filename()
{
    return Glib::build_filename(Glib::get_user_cache_dir(), "horizon-stock_info_provider_digikey_cache.db");
}

void StockInfoProviderDigiKey::init_db()
{
    SQLite::Database cache_db(get_db_filename(), SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
    int user_version = cache_db.get_user_version();
    if (user_version < min_user_version) {
        // update schema
        auto bytes = Gio::Resource::lookup_data_global(
                "/org/horizon-eda/horizon/util/"
                "stock_info_provider_digikey_schema.sql");
        gsize size{bytes->get_size() + 1}; // null byte
        auto data = (const char *)bytes->get_data(size);
        cache_db.execute(data);
    }
}

bool StockInfoProviderDigiKey::is_valid()
{
    const auto &prefs = PreferencesProvider::get_prefs().digikey_api;
    if (prefs.client_id.size() == 0 || prefs.client_secret.size() == 0)
        return false;
    SQLite::Database db(get_db_filename(), SQLITE_OPEN_READONLY, 1000);
    SQLite::Query q(db, "SELECT value FROM tokens WHERE key = 'refresh' AND valid_until > datetime()");
    return q.step();
}

void StockInfoProviderDigiKey::cleanup()
{
    SQLite::Database db(get_db_filename(), SQLITE_OPEN_READWRITE, 1000);
    db.execute("DELETE FROM cache");
}

static void update_token(SQLite::Database &db, const std::string &key, const std::string &value, int expiry_seconds)
{
    SQLite::Query q(db,
                    "INSERT OR REPLACE INTO tokens (key, value, valid_until) VALUES (?, ?, datetime('now', ? || ' "
                    "seconds'))");
    q.bind(1, key);
    q.bind(2, value);
    q.bind(3, expiry_seconds);
    q.step();
}

std::string StockInfoProviderDigiKey::update_tokens_from_response(SQLite::Database &db, const json &resp)
{
    const auto access_token = resp.at("access_token").get<std::string>();
    const auto new_refresh_token = resp.at("refresh_token").get<std::string>();
    const auto access_token_expiry = resp.at("expires_in").get<int>();
    const auto refresh_token_expiry = resp.at("refresh_token_expires_in").get<int>();
    update_token(db, "access", access_token, access_token_expiry - 60);
    update_token(db, "refresh", new_refresh_token, refresh_token_expiry);
    return access_token;
}

static int remaining_from_headers(const HTTP::Client::ResponseHeaders &headers)
{
    for (const auto &it : headers) {
        if (it.find("X-RateLimit-Remaining:") == 0) {
            std::string s = it.substr(22);
            trim(s);
            return std::stoi(s);
        }
    }
    return 0;
}

class DigiKeyCacheManager {
public:
    static std::shared_ptr<DigiKeyCacheManager> &get()
    {
        static std::mutex mutex;
        static std::shared_ptr<DigiKeyCacheManager> inst;
        std::lock_guard guard(mutex);
        if (!inst)
            inst.reset(new DigiKeyCacheManager);
        return inst;
    }
    ~DigiKeyCacheManager()
    {
        std::cout << "cache manager dtor" << std::endl;
    }


    std::pair<bool, json> query_cache(const std::string &MPN, const std::string &manufacturer)
    {
        SQLite::Query q(cache_db,
                        "SELECT data, last_updated FROM cache WHERE MPN=? AND manufacturer=? AND "
                        "last_updated > datetime('now', '-1 days')");
        q.bind(1, MPN);
        q.bind(2, manufacturer);
        if (q.step()) {
            return {true, json::parse(q.get<std::string>(0))};
        }
        else {
            return {false, nullptr};
        }
    }

    std::pair<json, unsigned int> query(const std::string &MPN, const std::string &manufacturer)
    {
        std::unique_lock<std::mutex> lk(mutex);
        unsigned int n_remaining = 0;

        json j;
        std::string access_token;
        try {
            cache_db.execute("BEGIN IMMEDIATE");
            SQLite::Query q_access(cache_db,
                                   "SELECT value FROM tokens WHERE key = 'access' AND valid_until > datetime()");
            if (q_access.step()) {
                access_token = q_access.get<std::string>(0);
            }
            else {
                SQLite::Query q_refresh(cache_db,
                                        "SELECT value FROM tokens WHERE key = 'refresh' AND valid_until > datetime()");
                if (q_refresh.step()) {
                    const auto refresh_token = q_refresh.get<std::string>(0);
                    HTTP::Client client;
                    std::string postdata = "client_id=" + prefs.client_id + "&client_secret=" + prefs.client_secret
                                           + "&grant_type=refresh_token&refresh_token=" + refresh_token;
                    const auto resp = json::parse(client.post("https://api.digikey.com/v1/oauth2/token", postdata));
                    access_token = StockInfoProviderDigiKey::update_tokens_from_response(cache_db, resp);
                }
                else {
                    std::cout << "no refresh token" << std::endl;
                }
            }
            cache_db.execute("COMMIT");
        }
        catch (const std::exception &e) {
            std::cerr << e.what() << std::endl;
            cache_db.execute("ROLLBACK");
            return {};
        }
        catch (...) {
            cache_db.execute("ROLLBACK");
            return {};
        }

        if (access_token.size()) {
            try {
                rest_client.clear_headers();
                rest_client.append_header("accept: application/json");
                rest_client.append_header("Content-Type: application/json");
                rest_client.append_header("Authorization: Bearer " + access_token);
                rest_client.append_header("X-DIGIKEY-Client-Id: " + prefs.client_id);
                rest_client.append_header("X-DIGIKEY-Locale-Site: " + prefs.site);
                rest_client.append_header("X-DIGIKEY-Locale-Language: en");
                rest_client.append_header("X-DIGIKEY-Locale-Currency: " + prefs.currency);
                json query = {
                        {"Keywords", MPN},
                        {"RecordCount", 50},
                        {"RecordStartPosition", 0},
                        {"Sort",
                         {
                                 {"SortOption", "SortByDigiKeyPartNumber"},
                                 {"Direction", "Ascending"},
                                 {"SortParameterId", 0},
                         }},
                        {"RequestedQuantity", 0},
                        {"SearchOptions", {"ManufacturerPartSearch"}},
                        {"ExcludeMarketPlaceProducts", true},
                };
                const json response = rest_client.post("Search/v3/Products/Keyword", query);
                n_remaining = remaining_from_headers(rest_client.get_response_headers());

                {
                    SQLite::Query q_insert(
                            cache_db,
                            "INSERT OR REPLACE INTO cache (MPN, manufacturer, data, last_updated) VALUES (?, "
                            "?, ?, datetime())");
                    q_insert.bind(1, MPN);
                    q_insert.bind(2, manufacturer);
                    q_insert.bind(3, response.dump());
                    q_insert.step();
                }
                return {response, n_remaining};
            }
            catch (const std::exception &e) {
                std::cout << e.what() << std::endl;
                return {};
            }
        }

        return {};
    }

private:
    DigiKeyCacheManager()
        : cache_db(StockInfoProviderDigiKey::get_db_filename(), SQLITE_OPEN_READWRITE, 10000),
          rest_client("https://api.digikey.com/"), prefs(PreferencesProvider::get_prefs().digikey_api)
    {
        rest_client.set_timeout(30);
    }

    SQLite::Database cache_db;
    HTTP::RESTClient rest_client;
    std::mutex mutex;
    DigiKeyApiPreferences prefs;
};

class StockInfoRecordDigiKey : public StockInfoRecord {
public:
    const UUID &get_uuid() const override
    {
        return uuid;
    }

    void append(const StockInfoRecord &aother) override
    {
        auto other = dynamic_cast<const StockInfoRecordDigiKey &>(aother);
        if (other.stock > stock) {
            stock = other.stock;
            state = other.state;
            for (auto &it : other.parts) {
                parts.push_front(it);
            }
        }
        else {
            for (auto &it : other.parts) {
                parts.push_back(it);
            }
        }
    }

    enum class State { FOUND, NOT_FOUND, NOT_AVAILABLE, NOT_LOADED };
    State state = State::FOUND;
    class OrderablePart {
    public:
        std::string MPN;
        json j;
    };
    std::list<OrderablePart> parts;
    int stock;
    std::string currency;
    UUID uuid;
};


class StockInfoProviderDigiKeyWorker {

public:
    StockInfoProviderDigiKeyWorker(const std::string &pool_base_path, Glib::Dispatcher &disp)
        : pool(pool_base_path), cache_mgr(DigiKeyCacheManager::get()), dispatcher(disp),
          prefs(PreferencesProvider::get_prefs().digikey_api)
    {
        fetch_thread = std::thread(&StockInfoProviderDigiKeyWorker::fetch_worker, this);
        auto worker_thread = std::thread(&StockInfoProviderDigiKeyWorker::worker_wrapper, this);
        worker_thread.detach();
    }
    std::list<std::shared_ptr<StockInfoRecord>> get_records();
    void update_parts(const std::list<UUID> &aparts);
    void load_more();
    void exit()
    {
        worker_exit = true;
        {
            std::unique_lock<std::mutex> lk(parts_mutex);
            parts.clear();
            parts_rev++;
        }
        {
            std::unique_lock<std::mutex> lk(parts_mutex_fetch);
            parts_fetch.clear();
            parts_rev_fetch++;
        }

        dispatcher_lock.lock();
        cond.notify_all();
        cond_fetch.notify_all();
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

    unsigned int get_n_remaining() const
    {
        return n_remaining;
    }

private:
    Pool pool;
    std::shared_ptr<class DigiKeyCacheManager> cache_mgr;
    void worker();
    void fetch_worker();
    void worker_wrapper()
    {
        worker();
        fetch_thread.join();
        delete this;
    }
    void add_record(const UUID &uu, const json &j, const std::string &MPN, const std::string &manufacturer);

    bool worker_exit = false;
    std::condition_variable cond;
    std::list<UUID> parts;
    unsigned int parts_rev = 0;
    std::mutex parts_mutex;

    std::condition_variable cond_fetch;
    std::list<std::tuple<UUID, std::string, std::string>> parts_fetch;
    unsigned int parts_rev_fetch = 0;
    std::mutex parts_mutex_fetch;

    std::mutex records_mutex;
    std::list<std::shared_ptr<StockInfoRecord>> records;
    std::mutex dispatcher_lock;
    Glib::Dispatcher &dispatcher;
    DigiKeyApiPreferences prefs;
    void emit();
    std::thread fetch_thread;

    std::atomic<unsigned int> n_items_from_cache = 0;
    std::atomic<unsigned int> n_items_to_fetch = 0;
    std::atomic<unsigned int> n_items_fetched = 0;
    std::atomic<unsigned int> n_remaining = 0;
};

void StockInfoProviderDigiKeyWorker::emit()
{
    if (dispatcher_lock.try_lock()) {
        dispatcher.emit();
        dispatcher_lock.unlock();
    }
}

static bool match_manufacturer(std::string haystack, std::string needle)
{
    std::transform(haystack.begin(), haystack.end(), haystack.begin(), tolower);
    std::transform(needle.begin(), needle.end(), needle.begin(), tolower);
    return haystack.find(needle) != std::string::npos;
}

static const json &find_product(const json &j, const std::string &manufacturer)
{
    static const json none = nullptr;
    for (const auto &o : j.at("ExactManufacturerProducts")) {
        const auto mfr = o.at("Manufacturer").at("Value").get<std::string>();
        if (match_manufacturer(mfr, manufacturer))
            return o;
    }
    for (const auto &o : j.at("Products")) {
        const auto mfr = o.at("Manufacturer").at("Value").get<std::string>();
        if (match_manufacturer(mfr, manufacturer))
            return o;
    }

    return none;
}

void StockInfoProviderDigiKeyWorker::add_record(const UUID &uu, const json &j, const std::string &MPN,
                                                const std::string &manufacturer)
{
    auto record = std::make_shared<StockInfoRecordDigiKey>();
    record->uuid = uu;
    if (j.is_null()) {
        record->state = StockInfoRecordDigiKey::State::NOT_FOUND;
    }
    else {
        int stock = -1;
        std::string url;
        try {
            const auto &o = find_product(j, manufacturer);
            if (!o.is_null()) {
                stock = o.at("QuantityAvailable").get<int>();
                record->stock = stock;
                record->currency = j.at("SearchLocaleUsed").at("Currency").get<std::string>();
                record->parts.emplace_back();
                record->parts.back().j = o;
                record->parts.back().MPN = MPN;
            }
        }
        catch (...) {
            stock = -1;
        }

        if (stock == -1) {
            record->state = StockInfoRecordDigiKey::State::NOT_AVAILABLE;
        }
    }
    {
        std::unique_lock<std::mutex> lk(records_mutex);
        records.emplace_back(record);
    }
}

void StockInfoProviderDigiKeyWorker::fetch_worker()
{
    unsigned int my_parts_rev = 0;
    while (1) {
        std::list<std::tuple<UUID, std::string, std::string>> my_parts;
        {
            std::unique_lock<std::mutex> lk(parts_mutex_fetch);
            cond_fetch.wait(lk, [this, &my_parts_rev] { return parts_rev_fetch != my_parts_rev; });
            my_parts_rev = parts_rev_fetch;
            my_parts = parts_fetch;
        }

        std::cout << "got fetch parts" << std::endl;
        bool cancel = false;
        n_items_to_fetch = my_parts.size();
        n_items_fetched = 0;
        emit();
        for (const auto &[uu, MPN, manufacturer] : my_parts) {
            n_items_fetched++;

            const auto [r, remaining] = cache_mgr->query(MPN, manufacturer);
            add_record(uu, r, MPN, manufacturer);
            n_remaining = remaining;

            if (my_parts_rev != parts_rev_fetch) {
                cancel = true;
                break;
            }

            if (worker_exit) {
                std::cout << "worker bye" << std::endl;
                return;
            }
            emit();
        }
        emit();
        if (cancel)
            continue;

        if (worker_exit) {
            std::cout << "fetch worker bye" << std::endl;
            return;
        }
    }
}

void StockInfoProviderDigiKeyWorker::worker()
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
        std::list<std::tuple<UUID, std::string, std::string>> parts_not_in_cache;
        bool cancel = false;
        n_items_from_cache = 0;
        for (const auto &it : my_parts) {
            SQLite::Query q(pool.db,
                            "SELECT parts.MPN, parts.manufacturer, orderable_MPNs.MPN FROM parts "
                            "LEFT JOIN orderable_MPNs ON parts.uuid = orderable_MPNs.part "
                            "WHERE parts.uuid=?");
            q.bind(1, it);
            while (q.step()) {
                std::string part_MPN = q.get<std::string>(0);
                std::string manufacturer = q.get<std::string>(1);
                std::string orderable_MPN = q.get<std::string>(2);
                std::string MPN;
                if (orderable_MPN.size())
                    MPN = orderable_MPN;
                else
                    MPN = part_MPN;

                const auto cr = cache_mgr->query_cache(MPN, manufacturer);
                if (cr.first == false) {
                    parts_not_in_cache.emplace_back(it, MPN, manufacturer);
                }
                else {
                    add_record(it, cr.second, MPN, manufacturer);
                    n_items_from_cache++;
                }

                if (my_parts_rev != parts_rev) {
                    cancel = true;
                    break;
                }
                if (worker_exit) {
                    std::cout << "worker bye" << std::endl;
                    return;
                }
            }
            emit();
        }
        if (cancel)
            continue;

        {
            {
                std::unique_lock<std::mutex> lk(parts_mutex_fetch);
                parts_fetch.clear();
                int max_fetch = 20;
                while (max_fetch && parts_not_in_cache.size()) {
                    parts_fetch.push_back(parts_not_in_cache.front());
                    parts_not_in_cache.pop_front();
                    max_fetch--;
                }
                parts_rev_fetch++;
            }
            cond_fetch.notify_all();
            std::cout << "notify fetch" << std::endl;
        }

        if (parts_not_in_cache.size()) {
            std::unique_lock<std::mutex> lk(records_mutex);

            for (const auto &[uu, MPN, manufacturer] : parts_not_in_cache) {
                auto record = std::make_shared<StockInfoRecordDigiKey>();
                record->uuid = uu;
                record->state = StockInfoRecordDigiKey::State::NOT_LOADED;
                records.emplace_back(record);
            }
            emit();
        }


        emit();
    }
    std::cout << "worker bye" << std::endl;
}

std::list<std::shared_ptr<StockInfoRecord>> StockInfoProviderDigiKeyWorker::get_records()
{
    std::list<std::shared_ptr<StockInfoRecord>> r;
    {
        std::unique_lock<std::mutex> lk(records_mutex);
        r = records;
        records.clear();
    }
    return r;
}

StockInfoProviderDigiKey::StockInfoProviderDigiKey(const std::string &pool_base_path)
    : worker(new StockInfoProviderDigiKeyWorker(pool_base_path, dispatcher))
{
    dispatcher.connect([this] {
        if (status_label) {
            std::string txt = "Digi-Key: " + format_digits(worker->get_n_items_from_cache(), 5) + " from cache";
            if (worker->get_n_items_to_fetch()) {
                txt += ", fetching " + format_m_of_n(worker->get_n_items_fetched(), worker->get_n_items_to_fetch());
            }
            if (auto remaining = worker->get_n_remaining()) {
                txt += ", " + std::to_string(remaining) + " queries remaining";
            }
            status_label->set_text(txt);
        }
    });
}

void StockInfoProviderDigiKey::update_parts(const std::list<UUID> &parts)
{
    worker->update_parts(parts);
}

std::list<std::shared_ptr<StockInfoRecord>> StockInfoProviderDigiKey::get_records()
{
    return worker->get_records();
}

void StockInfoProviderDigiKey::add_columns(Gtk::TreeView *atreeview,
                                           Gtk::TreeModelColumn<std::shared_ptr<StockInfoRecord>> acolumn)
{
    assert(!treeview);
    treeview = atreeview;
    column = acolumn;
    tvc = Gtk::manage(new Gtk::TreeViewColumn("Digi-Key Stock"));
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
            if (auto p = dynamic_cast<const StockInfoRecordDigiKey *>(v.get())) {
                if (p->state == StockInfoRecordDigiKey::State::FOUND) {
                    std::ostringstream oss;
                    oss.imbue(std::locale(""));
                    oss << p->stock;
                    mcr->property_text() = oss.str();
                }
                else if (p->state == StockInfoRecordDigiKey::State::NOT_AVAILABLE) {
                    mcr->property_text() = "N/A";
                }
                else if (p->state == StockInfoRecordDigiKey::State::NOT_LOADED) {
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

    popover.set_relative_to(*treeview);
    popover.set_position(Gtk::POS_BOTTOM);

    treeview->signal_button_press_event().connect_notify(sigc::mem_fun(*this, &StockInfoProviderDigiKey::handle_click));
}

void StockInfoProviderDigiKey::handle_click(GdkEventButton *ev)
{
    if (ev->button == 1) {
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
                if (auto ch = popover.get_child())
                    delete ch;

                auto v = treeview->get_model()->get_iter(path)->get_value(column);
                if (auto p = dynamic_cast<const StockInfoRecordDigiKey *>(v.get())) {
                    if (p->state == StockInfoRecordDigiKey::State::FOUND) {
                        construct_popover(*p);
                        popover.set_pointing_to(rect);
                        popover.popup();
                    }
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

void StockInfoProviderDigiKey::construct_popover(const StockInfoRecordDigiKey &rec)
{
    auto obox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 4));
    obox->property_margin() = 4;
    Gtk::ComboBoxText *combobox = nullptr;
    if (rec.parts.size() > 1) {
        combobox = Gtk::manage(new Gtk::ComboBoxText);
        obox->pack_start(*combobox, false, false, 0);
    }
    auto stack = Gtk::manage(new Gtk::Stack());
    obox->pack_start(*stack, true, true, 0);
    {
        auto la_info = Gtk::manage(new Gtk::Label);
        la_info->get_style_context()->add_class("dim-label");
        la_info->set_markup(
                "Information provided by the "
                "<a href='https://www.digikey.com/en/resources/api-solutions'>Digi-Key API</a>");
        obox->pack_start(*la_info, false, false, 0);
        auto attributes_list = pango_attr_list_new();
        auto attribute_scale = pango_attr_scale_new(.833);
        pango_attr_list_insert(attributes_list, attribute_scale);
        gtk_label_set_attributes(la_info->gobj(), attributes_list);
        pango_attr_list_unref(attributes_list);
    }
    unsigned int i_part = 0;
    for (const auto &it_part : rec.parts) {
        auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 4));
        box->property_margin() = 4;

        {
            std::string descr = it_part.MPN;
            descr = it_part.j.at("DetailedDescription");
            auto la = Gtk::manage(new Gtk::Label());
            la->set_max_width_chars(0);
            la->set_ellipsize(Pango::ELLIPSIZE_END);
            la->set_xalign(0);
            la->set_markup(
                    make_link_markup(it_part.j.at("PrimaryDatasheet"), descr) + "\n"
                    + make_link_markup(it_part.j.at("ProductUrl"), it_part.j.at("DigiKeyPartNumber").get<std::string>())
                    + "\n" + it_part.j.at("Packaging").at("Value").get<std::string>());
            box->pack_start(*la, false, false, 0);
        }

        {
            auto gr = Gtk::manage(new Gtk::Grid);
            gr->set_column_spacing(8);
            int top = 0;
            int max_price_breaks = PreferencesProvider::get_prefs().digikey_api.max_price_breaks;
            for (const auto &it_price : it_part.j.at("StandardPricing")) {
                const auto qty = it_price.at("BreakQuantity").get<int>();
                const auto price = it_price.at("UnitPrice").get<double>();
                auto la_qty = Gtk::manage(new Gtk::Label(std::to_string(qty)));
                la_qty->set_xalign(0);
                auto la_price = Gtk::manage(new Gtk::Label());
                label_set_tnum(la_qty);
                label_set_tnum(la_price);
                la_price->set_xalign(0);
                {
                    std::ostringstream oss;
                    oss << get_currency_sign(rec.currency) << std::fixed << std::setprecision(3) << price;
                    la_price->set_text(oss.str());
                }
                gr->attach(*la_qty, 0, top, 1, 1);
                gr->attach(*la_price, 1, top, 1, 1);
                top++;
                if (top == max_price_breaks)
                    break;
            }
            gr->set_margin_start(4);
            gr->set_margin_bottom(8);
            box->pack_start(*gr, false, false, 0);
        }

        box->show_all();
        stack->add(*box, std::to_string(i_part), it_part.MPN);
        if (combobox)
            combobox->append(std::to_string(i_part), it_part.MPN);
        i_part++;
    }
    if (combobox) {
        combobox->signal_changed().connect([combobox, stack] { stack->set_visible_child(combobox->get_active_id()); });
        if (i_part)
            combobox->set_active_id("0");
    }
    popover.add(*obox);
    obox->show_all();
}

void StockInfoProviderDigiKeyWorker::update_parts(const std::list<UUID> &aparts)
{
    {
        std::unique_lock<std::mutex> lk(parts_mutex);
        parts = aparts;
        parts_rev++;
    }
    cond.notify_all();
}

void StockInfoProviderDigiKeyWorker::load_more()
{
    {
        std::unique_lock<std::mutex> lk(parts_mutex);
        parts_rev++;
    }
    cond.notify_all();
}


StockInfoProviderDigiKey::~StockInfoProviderDigiKey()
{
    std::cout << "dtor" << std::endl;
    worker->exit();
}

Gtk::Widget *StockInfoProviderDigiKey::create_status_widget()
{
    status_label = Gtk::manage(new Gtk::Label(""));
    label_set_tnum(status_label);
    return status_label;
}

} // namespace horizon
