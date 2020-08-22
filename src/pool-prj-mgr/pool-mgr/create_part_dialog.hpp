#pragma once
#include <gtkmm.h>
#include "util/uuid.hpp"

namespace horizon {

class CreatePartDialog : public Gtk::Dialog {
public:
    CreatePartDialog(Gtk::Window *parent, class IPool &ipool, const UUID &entity_uuid, const UUID &package_uuid);
    UUID get_entity();
    UUID get_package();

private:
    class IPool &pool;
    class PoolBrowserEntity *browser_entity = nullptr;
    class PoolBrowserPackage *browser_package = nullptr;
    Gtk::Button *button_ok;
    void check_select();
    void check_activate();
};
} // namespace horizon
