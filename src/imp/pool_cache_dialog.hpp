#pragma once
#include <gtkmm.h>
#include "common/common.hpp"
#include <set>

namespace horizon {

class PoolCacheDialog : public Gtk::Dialog {
public:
    PoolCacheDialog(Gtk::Window *parent, class IPool &pool, const std::set<std::string> &items_modified);
    const auto &get_filenames() const
    {
        return filenames;
    }

private:
    class PoolCacheBox *cache_box;
    std::vector<std::string> filenames;
};
} // namespace horizon
