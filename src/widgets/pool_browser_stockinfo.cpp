#include "pool_browser_stockinfo.hpp"

namespace horizon {
void PoolBrowserStockinfo::add_stock_info_provider(std::unique_ptr<StockInfoProvider> prv)
{
    if (stock_info_provider)
        return;
    stock_info_provider = std::move(prv);
    stock_info_provider->add_columns(treeview, get_stock_info_column());
    {
        auto w = stock_info_provider->create_status_widget();
        if (w) {
            status_box->pack_start(*w, false, false, 0);
            w->show();
        }
    }
    stock_info_provider->dispatcher.connect([this] {
        auto records = stock_info_provider->get_records();
        for (const auto &it : records) {
            if (iter_cache.count(it->get_uuid())) {
                if (iter_cache.at(it->get_uuid())->get_value(get_stock_info_column()) == nullptr)
                    iter_cache.at(it->get_uuid())->set_value(get_stock_info_column(), it);
                else
                    iter_cache.at(it->get_uuid())->get_value(get_stock_info_column())->append(*it.get());
            }
        }
    });
}
} // namespace horizon
