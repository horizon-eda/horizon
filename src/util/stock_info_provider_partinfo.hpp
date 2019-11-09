#pragma once
#include "stock_info_provider.hpp"
#include "http_client.hpp"
#include <condition_variable>
#include <mutex>
#include <thread>
#include "nlohmann/json_fwd.hpp"
#include "pool/pool.hpp"

namespace horizon {
using json = nlohmann::json;

class StockInfoProviderPartinfo : public StockInfoProvider {
public:
    StockInfoProviderPartinfo(const std::string &pool_base_path);
    void add_columns(Gtk::TreeView *treeview, Gtk::TreeModelColumn<std::shared_ptr<StockInfoRecord>> column) override;
    void update_parts(const std::list<UUID> &parts) override;
    std::list<std::shared_ptr<StockInfoRecord>> get_records() override;
    Gtk::Widget *create_status_widget() override;
    ~StockInfoProviderPartinfo();

private:
    class StockInfoProviderPartinfoWorker *worker = nullptr;
    class StockInfoProviderPartinfoFetchWorker *fetch_worker = nullptr;
    Gtk::Label *status_label = nullptr;
    Gtk::Popover popover;
};
} // namespace horizon
