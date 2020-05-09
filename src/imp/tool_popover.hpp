#pragma once
#include "action.hpp"
#include "action_catalog.hpp"
#include <gtkmm.h>

namespace horizon {

class ToolPopover : public Gtk::Popover {
public:
    ToolPopover(Gtk::Widget *parent, ActionCatalogItem::Availability av);
    class ToolBox &get_tool_box();

private:
    class ToolBox *tool_box = nullptr;

    void on_show() override;
};
} // namespace horizon
