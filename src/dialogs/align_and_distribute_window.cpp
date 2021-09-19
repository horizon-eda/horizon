#include "align_and_distribute_window.hpp"
#include "util/gtk_util.hpp"
#include "util/util.hpp"
#include "imp/imp_interface.hpp"
#include <math.h>

namespace horizon {

using Operation = ToolAlignAndDistribute::Operation;

struct OperationInfo {
    std::string name;
    std::string icon;
};

static const std::map<ToolAlignAndDistribute::Operation, OperationInfo> operations = {
        {Operation::ALIGN_ORIGIN_TOP, {"Align centers top", "align-top-symbolic"}},
        {Operation::ALIGN_ORIGIN_BOTTOM, {"Align centers bottom", "align-bottom-symbolic"}},
        {Operation::ALIGN_ORIGIN_LEFT, {"Align centers left", "align-left-symbolic"}},
        {Operation::ALIGN_ORIGIN_RIGHT, {"Align centers right", "align-right-symbolic"}},
        {Operation::ALIGN_BBOX_TOP, {"Align top", "align-top-symbolic"}},
        {Operation::ALIGN_BBOX_BOTTOM, {"Align bottom", "align-bottom-symbolic"}},
        {Operation::ALIGN_BBOX_LEFT, {"Align left", "align-left-symbolic"}},
        {Operation::ALIGN_BBOX_RIGHT, {"Align right", "align-right-symbolic"}},
        {Operation::DISTRIBUTE_ORIGIN_HORIZONTAL, {"Distribute centers horizontally", "distribute-center-h-symbolic"}},
        {Operation::DISTRIBUTE_ORIGIN_VERTICAL, {"Distribute centers vertically", "distribute-center-v-symbolic"}},
        {Operation::DISTRIBUTE_EQUIDISTANT_HORIZONTAL, {"Distribute horizontally", "distribute-h-symbolic"}},
        {Operation::DISTRIBUTE_EQUIDISTANT_VERTICAL, {"Distribute vertically", "distribute-v-symbolic"}},
};

Gtk::Box *AlignAndDistributeWindow::make_box(const std::string &title, const std::vector<Operation> &ops)
{
    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 10));
    {
        auto la = Gtk::manage(new Gtk::Label);
        la->get_style_context()->add_class("dim-label");
        la->set_markup("<b>" + title + "</b>");
        box->pack_start(*la, false, false);
    }
    {
        auto b = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
        b->get_style_context()->add_class("linked");
        for (auto op : ops) {
            auto bu = make_button(op);
            b->pack_start(*bu, true, true);
        }
        box->pack_start(*b, false, false);
    }
    box->show_all();
    return box;
}

AlignAndDistributeWindow::AlignAndDistributeWindow(Gtk::Window *parent, ImpInterface *intf) : ToolWindow(parent, intf)
{
    set_title("Align and distribute");
    auto gr = Gtk::manage(new Gtk::Grid());
    gr->property_margin() = 20;
    gr->set_row_spacing(20);
    gr->set_column_spacing(20);
    using O = Operation;
    {
        auto b = make_box("Align bounding boxes",
                          {O::ALIGN_BBOX_LEFT, O::ALIGN_BBOX_BOTTOM, O::ALIGN_BBOX_TOP, O::ALIGN_BBOX_RIGHT});
        gr->attach(*b, 0, 0, 1, 1);
    }
    {
        auto b = make_box("Align centers",
                          {O::ALIGN_ORIGIN_LEFT, O::ALIGN_ORIGIN_BOTTOM, O::ALIGN_ORIGIN_TOP, O::ALIGN_ORIGIN_RIGHT});
        gr->attach(*b, 0, 1, 1, 1);
    }
    {
        auto b = make_box("Distribute", {O::DISTRIBUTE_EQUIDISTANT_HORIZONTAL, O::DISTRIBUTE_EQUIDISTANT_VERTICAL});
        gr->attach(*b, 1, 0, 1, 1);
    }
    {
        auto b = make_box("Distribute centers", {O::DISTRIBUTE_ORIGIN_HORIZONTAL, O::DISTRIBUTE_ORIGIN_VERTICAL});
        gr->attach(*b, 1, 1, 1, 1);
    }
    gr->show_all();
    add(*gr);
}


Gtk::Button *AlignAndDistributeWindow::make_button(Operation op)
{
    auto bu = Gtk::manage(new Gtk::Button);
    auto &x = operations.at(op);
    bu->add_events(Gdk::ENTER_NOTIFY_MASK | Gdk::LEAVE_NOTIFY_MASK);
    bu->set_tooltip_text(x.name);
    bu->set_image_from_icon_name(x.icon, Gtk::ICON_SIZE_BUTTON);
    bu->signal_clicked().connect([this, op] { emit(op, false); });
    bu->signal_leave_notify_event().connect_notify([this](GdkEventCrossing *ev) { emit(Operation::RESET, true); });
    bu->signal_enter_notify_event().connect_notify([this, op](GdkEventCrossing *ev) { emit(op, true); });
    return bu;
}

void AlignAndDistributeWindow::emit(Operation op, bool preview)
{
    auto data = std::make_unique<ToolDataAlignAndDistribute>();
    data->event = ToolDataWindow::Event::UPDATE;
    data->operation = op;
    data->preview = preview;
    interface->tool_update_data(std::move(data));
}

} // namespace horizon
