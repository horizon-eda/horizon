#pragma once
#include "stock_info_provider.hpp"
#include "http_client.hpp"
#include <condition_variable>
#include <mutex>
#include <thread>
#include "nlohmann/json_fwd.hpp"
#include "pool/pool.hpp"

namespace horizon {
namespace SQLite {
class Database;
}

using json = nlohmann::json;

class StockInfoProviderDigiKey : public StockInfoProvider {
public:
    StockInfoProviderDigiKey(const std::string &pool_base_path);
    void add_columns(Gtk::TreeView *treeview, Gtk::TreeModelColumn<std::shared_ptr<StockInfoRecord>> column) override;
    void update_parts(const std::list<UUID> &parts) override;
    std::list<std::shared_ptr<StockInfoRecord>> get_records() override;
    Gtk::Widget *create_status_widget() override;
    ~StockInfoProviderDigiKey();
    static void init_db();
    static std::string get_db_filename();
    static std::string update_tokens_from_response(SQLite::Database &db, const json &j);
    static bool is_valid();

private:
    class StockInfoProviderDigiKeyWorker *worker = nullptr;
    class StockInfoProviderDigiKeyFetchWorker *fetch_worker = nullptr;
    Gtk::Label *status_label = nullptr;
    Gtk::Popover popover;

    void handle_click(GdkEventButton *ev);
    void construct_popover(const class StockInfoRecordDigiKey &rec);
    Gtk::TreeView *treeview = nullptr;
    Gtk::TreeViewColumn *tvc = nullptr;
    Gtk::TreeModelColumn<std::shared_ptr<StockInfoRecord>> column;
};
} // namespace horizon
