#include "stock_info_provider.hpp"
#include "preferences/preferences_provider.hpp"
#include "preferences/preferences.hpp"
#include "stock_info_provider_partinfo.hpp"
#include "stock_info_provider_digikey.hpp"

namespace horizon {
std::unique_ptr<StockInfoProvider> StockInfoProvider::create(const std::string &pool_base_path)
{
    const auto &prefs = PreferencesProvider::get_prefs();
    switch (prefs.stock_info_provider) {
    case Preferences::StockInfoProviderSel::PARTINFO:
        if (prefs.partinfo.preferred_distributor.size()) {
            return std::make_unique<StockInfoProviderPartinfo>(pool_base_path);
        }
        break;

    case Preferences::StockInfoProviderSel::NONE:
        return nullptr;

    case Preferences::StockInfoProviderSel::DIGIKEY:
        if (StockInfoProviderDigiKey::is_valid())
            return std::make_unique<StockInfoProviderDigiKey>(pool_base_path);
        break;
    }
    return nullptr;
}

void StockInfoProvider::init_db()
{
    StockInfoProviderDigiKey::init_db();
}

void StockInfoProvider::cleanup()
{
    StockInfoProviderDigiKey::cleanup();
}

} // namespace horizon
