#include "stock_info_provider_partinfo.hpp"
#include <thread>
#include "nlohmann/json.hpp"
#include "preferences/preferences_provider.hpp"
#include "util/gtk_util.hpp"
#include "util/util.hpp"
#ifdef G_OS_WIN32
#undef ERROR
#undef DELETE
#undef DUPLICATE
#endif
#include "preferences/preferences.hpp"
#include <list>
#include <iomanip>

namespace horizon {

static const int min_user_version = 1; // keep in sync with schema

class PartInfoCacheManager {
public:
    static PartInfoCacheManager &get()
    {
        static PartInfoCacheManager inst;
        return inst;
    }


    std::pair<bool, json> query_cache(const std::string &MPN, const std::string &manufacturer)
    {
        SQLite::Query q(cache_db,
                        "SELECT data, last_updated FROM cache WHERE MPN=? AND manufacturer=? AND "
                        "last_updated > datetime('now', ?)");
        q.bind(1, MPN);
        q.bind(2, manufacturer);
        q.bind(3, "-" + std::to_string(prefs.cache_days) + " days");
        if (q.step()) {
            return {true, json::parse(q.get<std::string>(0))};
        }
        else {
            return {false, nullptr};
        }
    }

    std::map<UUID, json> query(const std::map<UUID, std::pair<std::string, std::string>> &parts)
    {
        std::unique_lock<std::mutex> lk(mutex);
        std::string q = "{ match(parts: [";
        for (const auto &it : parts) {
            q += "{mpn: {part: \"" + it.second.first + "\", manufacturer: \"" + it.second.second + "\"}},";
        }
        q += ",]) {\
        		datasheet\
    description\
    type\
    offers {\
      in_stock_quantity\
      moq\
      product_url\
		sku {\
        vendor\
        part\
      }\
      prices {\
        USD\
        EUR\
        GBP\
        SGD\
      }\
    }\
        		}}";
        std::string url = prefs.url + "?query=" + Glib::uri_escape_string(q);
        json j;
        try {
            auto resp = http_client.get(url);
            j = json::parse(resp);
        }
        catch (...) {
            return {};
        }
        std::map<UUID, json> r;
        {
            size_t i = 0;
            for (const auto &it : parts) {
                const json &k = j.at("data").at("match").at(i);
                {
                    SQLite::Query q2(cache_db,
                                     "INSERT OR REPLACE INTO cache (MPN, manufacturer, data, last_updated) VALUES (?, "
                                     "?, ?, datetime())");
                    q2.bind(1, it.second.first);
                    q2.bind(2, it.second.second);
                    q2.bind(3, k.dump());
                    q2.step();
                }
                r.emplace(it.first, k);
                i++;
            }
        }
        return r;
    }

private:
    PartInfoCacheManager()
        : cache_db(Glib::build_filename(Glib::get_user_cache_dir(), "horizon-stock_info_provider_part_info_cache.db"),
                   SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, 10000),
          prefs(PreferencesProvider::get_prefs().partinfo)
    {
        int user_version = cache_db.get_user_version();
        if (user_version < min_user_version) {
            // update schema
            auto bytes = Gio::Resource::lookup_data_global(
                    "/org/horizon-eda/horizon/util/"
                    "stock_info_provider_partinfo_schema.sql");
            gsize size{bytes->get_size() + 1}; // null byte
            auto data = (const char *)bytes->get_data(size);
            cache_db.execute(data);
        }
        http_client.set_timeout(30);
    }

    SQLite::Database cache_db;
    HTTP::Client http_client;
    std::mutex mutex;
    PartInfoPreferences prefs;
};

class StockInfoRecordPartinfo : public StockInfoRecord {
public:
    const UUID &get_uuid() const
    {
        return uuid;
    }

    void append(const StockInfoRecord &aother) override
    {
        auto other = dynamic_cast<const StockInfoRecordPartinfo &>(aother);
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
    UUID uuid;
};


class StockInfoProviderPartinfoWorker {

public:
    StockInfoProviderPartinfoWorker(const std::string &pool_base_path, Glib::Dispatcher &disp)
        : pool(pool_base_path), cache_mgr(PartInfoCacheManager::get()), dispatcher(disp),
          prefs(PreferencesProvider::get_prefs().partinfo)
    {
        fetch_thread = std::thread(&StockInfoProviderPartinfoWorker::fetch_worker, this);
        auto worker_thread = std::thread(&StockInfoProviderPartinfoWorker::worker_wrapper, this);
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

private:
    Pool pool;
    class PartInfoCacheManager &cache_mgr;
    void worker();
    void fetch_worker();
    void worker_wrapper()
    {
        worker();
        fetch_thread.join();
        delete this;
    }
    void add_record(const UUID &uu, const json &j, const std::string &MPN);

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
    PartInfoPreferences prefs;
    void emit();
    std::thread fetch_thread;

    unsigned int n_items_from_cache = 0;
    unsigned int n_items_to_fetch = 0;
    unsigned int n_items_fetched = 0;
};

void StockInfoProviderPartinfoWorker::emit()
{
    if (dispatcher_lock.try_lock()) {
        dispatcher.emit();
        dispatcher_lock.unlock();
    }
}

void StockInfoProviderPartinfoWorker::add_record(const UUID &uu, const json &j, const std::string &MPN)
{
    auto record = std::make_shared<StockInfoRecordPartinfo>();
    record->uuid = uu;
    if (j.is_null()) {
        record->state = StockInfoRecordPartinfo::State::NOT_FOUND;
    }
    else {
        int stock = -1;
        std::string url;
        try {
            const auto &offers = j.at("offers");
            for (const auto &offer : offers) {
                int moq = 1;
                if (offer.at("moq").is_number_integer())
                    moq = offer.at("moq");
                if (moq > 1 && prefs.ignore_moq_gt_1)
                    continue;

                if (offer.at("sku").at("vendor") == prefs.preferred_distributor) {
                    int qty = offer.at("in_stock_quantity");
                    if (qty > stock) {
                        stock = qty;
                        url = offer.at("product_url");
                    }
                }
            }
        }
        catch (...) {
            stock = -1;
        }

        if (stock == -1) {
            record->state = StockInfoRecordPartinfo::State::NOT_AVAILABLE;
        }
        record->stock = stock;
        record->parts.emplace_back();
        record->parts.back().j = j;
        record->parts.back().MPN = MPN;
    }
    {
        std::unique_lock<std::mutex> lk(records_mutex);
        records.emplace_back(record);
    }
}

void StockInfoProviderPartinfoWorker::fetch_worker()
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
        std::map<UUID, std::pair<std::string, std::string>> parts_temp;
        n_items_to_fetch = my_parts.size();
        n_items_fetched = 0;
        emit();
        for (const auto &it : my_parts) {
            UUID uu;
            std::string MPN, manufacturer;
            std::tie(uu, MPN, manufacturer) = it;
            parts_temp.emplace(std::piecewise_construct, std::forward_as_tuple(uu),
                               std::forward_as_tuple(MPN, manufacturer));
            n_items_fetched++;
            if (parts_temp.size() >= 20) {
                auto r = cache_mgr.query(parts_temp);
                for (const auto &it2 : r) {
                    add_record(it2.first, it2.second, parts_temp.at(it2.first).first);
                }
                if (my_parts_rev != parts_rev_fetch) {
                    cancel = true;
                    break;
                }

                if (worker_exit) {
                    std::cout << "worker bye" << std::endl;
                    return;
                }
                emit();
                parts_temp.clear();
            }
        }
        emit();
        if (cancel)
            continue;


        else if (parts_temp.size()) {
            auto r = cache_mgr.query(parts_temp);
            if (my_parts_rev != parts_rev_fetch)
                continue;
            if (worker_exit) {
                std::cout << "worker bye" << std::endl;
                return;
            }
            for (const auto &it2 : r) {
                add_record(it2.first, it2.second, parts_temp.at(it2.first).first);
            }
            emit();
        }


        if (worker_exit) {
            std::cout << "fetch worker bye" << std::endl;
            return;
        }
    }
}

void StockInfoProviderPartinfoWorker::worker()
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

                const auto cr = cache_mgr.query_cache(MPN, manufacturer);
                if (cr.first == false) {
                    parts_not_in_cache.emplace_back(it, MPN, manufacturer);
                }
                else {
                    add_record(it, cr.second, MPN);
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
                int max_fetch = 200;
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

            for (const auto &it : parts_not_in_cache) {
                auto record = std::make_shared<StockInfoRecordPartinfo>();
                UUID uu;
                std::string MPN, manufacturer;
                std::tie(uu, MPN, manufacturer) = it;
                record->uuid = uu;
                record->state = StockInfoRecordPartinfo::State::NOT_LOADED;
                records.emplace_back(record);
            }
            emit();
        }


        emit();
    }
    std::cout << "worker bye" << std::endl;
}

std::list<std::shared_ptr<StockInfoRecord>> StockInfoProviderPartinfoWorker::get_records()
{
    std::list<std::shared_ptr<StockInfoRecord>> r;
    {
        std::unique_lock<std::mutex> lk(records_mutex);
        r = records;
        records.clear();
    }
    return r;
}

StockInfoProviderPartinfo::StockInfoProviderPartinfo(const std::string &pool_base_path)
    : worker(new StockInfoProviderPartinfoWorker(pool_base_path, dispatcher))
{
    dispatcher.connect([this] {
        if (status_label) {
            std::string txt = "Partinfo: " + format_digits(worker->get_n_items_from_cache(), 5) + " from cache";
            if (worker->get_n_items_to_fetch()) {
                txt += ", fetching " + format_m_of_n(worker->get_n_items_fetched(), worker->get_n_items_to_fetch());
            }
            status_label->set_text(txt);
        }
    });
}

void StockInfoProviderPartinfo::update_parts(const std::list<UUID> &parts)
{
    worker->update_parts(parts);
}

std::list<std::shared_ptr<StockInfoRecord>> StockInfoProviderPartinfo::get_records()
{
    return worker->get_records();
}

void StockInfoProviderPartinfo::add_columns(Gtk::TreeView *treeview,
                                            Gtk::TreeModelColumn<std::shared_ptr<StockInfoRecord>> column)
{
    auto tvc = Gtk::manage(
            new Gtk::TreeViewColumn(PreferencesProvider::get_prefs().partinfo.preferred_distributor + " Stock"));
    auto cr = Gtk::manage(new Gtk::CellRendererText());
    {
        auto attributes_list = pango_attr_list_new();
        auto attribute_font_features = pango_attr_font_features_new("tnum 1");
        pango_attr_list_insert(attributes_list, attribute_font_features);
        g_object_set(G_OBJECT(cr->gobj()), "attributes", attributes_list, NULL);
        pango_attr_list_unref(attributes_list);
    }
    tvc->set_cell_data_func(*cr, [column](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
        Gtk::TreeModel::Row row = *it;
        auto mcr = dynamic_cast<Gtk::CellRendererText *>(tcr);
        auto v = row.get_value(column);
        if (v == nullptr) {
            mcr->property_text() = "Loading...";
        }
        else {
            if (auto p = dynamic_cast<const StockInfoRecordPartinfo *>(v.get())) {
                if (p->state == StockInfoRecordPartinfo::State::FOUND) {
                    std::ostringstream oss;
                    oss.imbue(std::locale(""));
                    oss << p->stock;
                    mcr->property_text() = oss.str();
                }
                else if (p->state == StockInfoRecordPartinfo::State::NOT_AVAILABLE) {
                    mcr->property_text() = "N/A";
                }
                else if (p->state == StockInfoRecordPartinfo::State::NOT_LOADED) {
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
    treeview->signal_button_press_event().connect_notify([this, treeview, tvc, cr, column](GdkEventButton *ev) {
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
                    if (auto p = dynamic_cast<const StockInfoRecordPartinfo *>(v.get())) {
                        if (p->state != StockInfoRecordPartinfo::State::NOT_FOUND
                            && p->state != StockInfoRecordPartinfo::State::NOT_LOADED) {
                            auto obox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 4));
                            obox->property_margin() = 4;
                            Gtk::ComboBoxText *combobox = nullptr;
                            if (p->parts.size() > 1) {
                                combobox = Gtk::manage(new Gtk::ComboBoxText);
                                obox->pack_start(*combobox, false, false, 0);
                            }
                            auto stack = Gtk::manage(new Gtk::Stack());
                            obox->pack_start(*stack, true, true, 0);
                            {
                                auto la_info = Gtk::manage(new Gtk::Label);
                                la_info->get_style_context()->add_class("dim-label");
                                la_info->set_markup(
                                        "Information provided by "
                                        "<a href='https://github.com/kitspace/partinfo'>partinfo</a>");
                                obox->pack_start(*la_info, false, false, 0);
                                auto attributes_list = pango_attr_list_new();
                                auto attribute_scale = pango_attr_scale_new(.833);
                                pango_attr_list_insert(attributes_list, attribute_scale);
                                gtk_label_set_attributes(la_info->gobj(), attributes_list);
                                pango_attr_list_unref(attributes_list);
                            }
                            unsigned int i_part = 0;
                            for (const auto &it_part : p->parts) {
                                auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 4));
                                box->property_margin() = 4;
                                if (it_part.j.is_object()) {
                                    {
                                        std::string descr = "Datasheet";
                                        if (it_part.j.at("description").is_string())
                                            descr = it_part.j.at("description");
                                        auto la = Gtk::manage(new Gtk::Label());
                                        la->set_max_width_chars(0);
                                        la->set_ellipsize(Pango::ELLIPSIZE_END);
                                        la->set_xalign(0);
                                        if (it_part.j.at("datasheet").is_string()) {
                                            std::string ds_url = it_part.j.at("datasheet");
                                            la->set_markup(make_link_markup(ds_url, descr));
                                        }
                                        else {
                                            la->set_text(descr);
                                        }


                                        box->pack_start(*la, false, false, 0);
                                    }
                                    auto sg_offer = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);
                                    bool no_offers = true;
                                    std::vector<json> offers = it_part.j.at("offers");
                                    std::sort(offers.begin(), offers.end(), [](const json &a, const json &b) {
                                        const auto &preferred =
                                                PreferencesProvider::get_prefs().partinfo.preferred_distributor;
                                        std::string vendor_a = a.at("sku").at("vendor");
                                        std::string vendor_b = b.at("sku").at("vendor");
                                        if (vendor_a == preferred)
                                            return true;
                                        else if (vendor_b == preferred)
                                            return false;
                                        if (vendor_a == vendor_b) {
                                            int stock_qty_a = a.at("in_stock_quantity");
                                            int stock_qty_b = b.at("in_stock_quantity");
                                            return stock_qty_b < stock_qty_a;
                                        }
                                        else {
                                            return vendor_a < vendor_b;
                                        }
                                    });
                                    for (const auto &offer : offers) {
                                        int moq = 1;
                                        if (offer.at("moq").is_number_integer())
                                            moq = offer.at("moq");
                                        if (moq > 1 && PreferencesProvider::get_prefs().partinfo.ignore_moq_gt_1)
                                            continue;
                                        no_offers = false;
                                        auto hbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4));
                                        {
                                            auto la = Gtk::manage(new Gtk::Label());
                                            std::string vendor = offer.at("sku").at("vendor");
                                            std::string sku = offer.at("sku").at("part");
                                            std::string prod_url = offer.at("product_url");
                                            la->set_markup("<b>" + Glib::Markup::escape_text(vendor) + "</b> "
                                                           + make_link_markup(prod_url, sku));
                                            la->set_xalign(0);
                                            sg_offer->add_widget(*la);
                                            hbox->pack_start(*la, false, false, 0);
                                        }
                                        {
                                            auto la = Gtk::manage(new Gtk::Label());
                                            int stock_qty = offer.at("in_stock_quantity");
                                            std::ostringstream oss;
                                            oss.imbue(std::locale(""));
                                            oss << "Stock: " << stock_qty;
                                            la->set_text(oss.str());
                                            la->set_xalign(0);
                                            hbox->pack_start(*la, false, false, 0);
                                        }
                                        box->pack_start(*hbox, false, false, 0);
                                        static const std::vector<std::pair<std::string, std::string>> currencies = {
                                                {"USD", "$"}, {"GBP", "£"}};
                                        for (const auto &it_currency : currencies) {
                                            if (offer.at("prices").count(it_currency.first)
                                                && offer.at("prices").at(it_currency.first).is_array()) {
                                                auto gr = Gtk::manage(new Gtk::Grid);
                                                gr->set_column_spacing(8);
                                                int top = 0;
                                                int max_price_breaks =
                                                        PreferencesProvider::get_prefs().partinfo.max_price_breaks;
                                                for (const auto &it_price : offer.at("prices").at(it_currency.first)) {
                                                    int qty = it_price.at(0);
                                                    double price = it_price.at(1);
                                                    auto la_qty = Gtk::manage(new Gtk::Label(std::to_string(qty)));
                                                    la_qty->set_xalign(0);
                                                    auto la_price = Gtk::manage(new Gtk::Label());
                                                    label_set_tnum(la_qty);
                                                    label_set_tnum(la_price);
                                                    la_price->set_xalign(0);
                                                    {
                                                        std::ostringstream oss;
                                                        oss << it_currency.second << std::fixed << std::setprecision(3)
                                                            << price;
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
                                                break;
                                            }
                                        }
                                    }
                                    if (no_offers) { // no offers
                                        auto la = Gtk::manage(new Gtk::Label);
                                        la->set_markup("<i>No offers for this part.</i>");
                                        box->pack_start(*la, false, false, 0);
                                    }
                                }
                                else if (it_part.j.is_null()) { // not found
                                    auto la = Gtk::manage(new Gtk::Label);
                                    la->set_markup("<i>Part not found.</i>");
                                    box->pack_start(*la, false, false, 0);
                                }
                                box->show_all();
                                stack->add(*box, std::to_string(i_part), it_part.MPN);
                                if (combobox)
                                    combobox->append(std::to_string(i_part), it_part.MPN);
                                i_part++;
                            }
                            if (combobox) {
                                combobox->signal_changed().connect([this, combobox, stack] {
                                    stack->set_visible_child(combobox->get_active_id());
                                });
                                if (i_part)
                                    combobox->set_active_id("0");
                            }
                            popover.add(*obox);
                            obox->show_all();
                            popover.set_pointing_to(rect);
                            popover.popup();
                        }
                    }
                }
            }
        }
    });
}

void StockInfoProviderPartinfoWorker::update_parts(const std::list<UUID> &aparts)
{
    {
        std::unique_lock<std::mutex> lk(parts_mutex);
        parts = aparts;
        parts_rev++;
    }
    cond.notify_all();
}

void StockInfoProviderPartinfoWorker::load_more()
{
    {
        std::unique_lock<std::mutex> lk(parts_mutex);
        parts_rev++;
    }
    cond.notify_all();
}


StockInfoProviderPartinfo::~StockInfoProviderPartinfo()
{
    std::cout << "dtor" << std::endl;
    worker->exit();
}

Gtk::Widget *StockInfoProviderPartinfo::create_status_widget()
{
    status_label = Gtk::manage(new Gtk::Label(""));
    label_set_tnum(status_label);
    return status_label;
}

} // namespace horizon
