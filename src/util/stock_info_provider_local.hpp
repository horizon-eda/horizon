#pragma once
#include "stock_info_provider.hpp"
#include "http_client.hpp"
#include <condition_variable>
#include <mutex>
#include <thread>
#include "nlohmann/json_fwd.hpp"
#include "pool/pool.hpp"
#include <glibmm/datetime.h>

namespace horizon {
namespace SQLite {
class Database;
}

using json = nlohmann::json;

class StockInfoRecordLocal : public StockInfoRecord {
public:
    StockInfoRecordLocal(const UUID &uu);
    const UUID &get_uuid() const override
    {
        return uuid;
    }

    void append(const StockInfoRecord &aother) override
    {
        auto other = dynamic_cast<const StockInfoRecordLocal &>(aother);
        if (other.stock >= stock) {
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
    UUID uuid;
    enum class State { FOUND, NOT_FOUND, NOT_AVAILABLE, NOT_LOADED };
    State state = State::NOT_LOADED;
    class OrderablePart {
    public:
        std::string MPN;
        json j;
    };
    std::list<OrderablePart> parts;
    int stock = -1;
    double price = 0;
    std::string location = "";
    Glib::DateTime last_updated;


    // Load stock info from the local database
    bool load();

    // Save stock info into local database
    bool save();
};

class StockInfoProviderLocal : public StockInfoProvider {
public:
    StockInfoProviderLocal(const std::string &pool_base_path);
    void add_columns(Gtk::TreeView *treeview, Gtk::TreeModelColumn<std::shared_ptr<StockInfoRecord>> column) override;
    void update_parts(const std::list<UUID> &parts) override;
    std::list<std::shared_ptr<StockInfoRecord>> get_records() override;
    Gtk::Widget *create_status_widget() override;
    ~StockInfoProviderLocal();

private:
    Gtk::Label *status_label = nullptr;
    std::list<std::shared_ptr<StockInfoRecord>> records;
    Pool pool;
    int n_in_stock;

    void handle_click(GdkEventButton *ev);
    Gtk::TreeView *treeview = nullptr;
    Gtk::TreeViewColumn *tvc = nullptr;
    Gtk::TreeModelColumn<std::shared_ptr<StockInfoRecord>> column;
};

} // namespace horizon
