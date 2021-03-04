#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common/common.hpp"
#include "parameter/set.hpp"

namespace horizon {

class BoardHoleDialog : public Gtk::Dialog {
public:
    BoardHoleDialog(Gtk::Window *parent, std::set<class BoardHole *> &pads, class IPool &p, class IPool &pool_caching,
                    class Block &block);
    bool valid = false;


private:
    class IPool &pool;
    class IPool &pool_caching;
    class Block &block;
    class ParameterSetEditor *editor = nullptr;
    class PoolBrowserButton *padstack_button = nullptr;
    class NetButton *net_button = nullptr;
};
} // namespace horizon
