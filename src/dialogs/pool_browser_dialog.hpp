#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common/common.hpp"
#include "util/uuid.hpp"
#include "util/uuid_path.hpp"
namespace horizon {


class PoolBrowserDialog : public Gtk::Dialog {
public:
    PoolBrowserDialog(Gtk::Window *parent, ObjectType type, class IPool &ipool, bool use_preview = true);
    class PoolBrowser &get_browser();

private:
    class IPool &pool;
    class PoolBrowser *browser = nullptr;
};
} // namespace horizon
