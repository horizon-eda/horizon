#include "tool_popover.hpp"
#include "action_catalog.hpp"
#include "util/str_util.hpp"
#include "tool_box.hpp"

namespace horizon {
ToolPopover::ToolPopover(Gtk::Widget *parent, ActionCatalogItem::Availability availability) : Gtk::Popover(*parent)
{
    tool_box = Gtk::manage(new ToolBox(availability));
    add(*tool_box);
    tool_box->show_all();
    tool_box->signal_action_activated().connect([this](auto act) {
#if GTK_CHECK_VERSION(3, 22, 0)
        popdown();
#else
        hide();
#endif
    });
}

void ToolPopover::on_show()
{
    Gtk::Popover::on_show();
    tool_box->select_all();
}

class ToolBox &ToolPopover::get_tool_box()
{
    return *tool_box;
}


} // namespace horizon
