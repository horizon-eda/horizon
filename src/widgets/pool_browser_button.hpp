#pragma once
#include <gtkmm.h>
#include "util/uuid.hpp"
#include "common/common.hpp"
#include "dialogs/pool_browser_dialog.hpp"

namespace horizon {

class PoolBrowserButton : public Gtk::Button {
public:
    PoolBrowserButton(ObjectType type, IPool &ipool);
    class PoolBrowser &get_browser();
    Glib::PropertyProxy<horizon::UUID> property_selected_uuid()
    {
        return p_property_selected_uuid.get_proxy();
    }

private:
    Glib::Property<UUID> p_property_selected_uuid;
    IPool &pool;
    ObjectType type;
    UUID selected_uuid;
    PoolBrowserDialog dia;
    void on_clicked() override;
    void update_label();
};
} // namespace horizon
