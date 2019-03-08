#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common/common.hpp"
#include "util/uuid.hpp"
#include "util/uuid_path.hpp"
#include "pool/pool.hpp"
namespace horizon {


class CreatePartDialog : public Gtk::Dialog {
public:
    CreatePartDialog(Gtk::Window *parent, Pool *ipool, const UUID &entity_uuid, const UUID &package_uuid);
    UUID get_entity();
    UUID get_package();

private:
    Pool *pool;
    class PoolBrowserEntity *browser_entity = nullptr;
    class PoolBrowserPackage *browser_package = nullptr;
    Gtk::Button *button_ok;
    void check_select();
    void check_activate();
};
} // namespace horizon
