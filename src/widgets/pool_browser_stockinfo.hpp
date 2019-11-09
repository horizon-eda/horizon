#pragma once
#include "util/stock_info_provider.hpp"
#include "pool_browser.hpp"
#include <memory>

namespace horizon {
class PoolBrowserStockinfo : public PoolBrowser {
public:
    using PoolBrowser::PoolBrowser;
    void add_stock_info_provider(std::unique_ptr<StockInfoProvider> prv);

protected:
    std::unique_ptr<StockInfoProvider> stock_info_provider;
    std::map<UUID, Gtk::TreeModel::iterator> iter_cache;
    virtual Gtk::TreeModelColumn<std::shared_ptr<StockInfoRecord>> &get_stock_info_column() = 0;
};
} // namespace horizon
