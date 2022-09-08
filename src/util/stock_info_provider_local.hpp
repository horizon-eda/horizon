#pragma once
#include "stock_info_provider.hpp"
#include "pool/pool.hpp"
#include <glibmm/datetime.h>

namespace horizon {
namespace SQLite {
class Database;
}

class StockInfoRecordLocal : public StockInfoRecord {
public:
    StockInfoRecordLocal(const UUID &uu);
    const UUID &get_uuid() const override
    {
        return uuid;
    }

    UUID uuid;
    enum class State { FOUND, NOT_FOUND, NOT_AVAILABLE, NOT_LOADED };
    State state = State::NOT_LOADED;
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
