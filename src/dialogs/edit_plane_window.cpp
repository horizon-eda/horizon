#include "edit_plane_window.hpp"
#include "widgets/net_button.hpp"
#include "widgets/spin_button_dim.hpp"
#include "widgets/plane_editor.hpp"
#include "board/board.hpp"
#include "util/gtk_util.hpp"

namespace horizon {

void bind(Gtk::Switch *sw, bool &v)
{
    sw->set_active(v);
    sw->property_active().signal_changed().connect([sw, &v] { v = sw->get_active(); });
}

EditPlaneWindow::EditPlaneWindow(Gtk::Window *parent, ImpInterface *intf, Plane &p, Board &b)
    : ToolWindow(parent, intf), plane(p), plane_uuid(plane.uuid), brd(b), poly(*plane.polygon)
{
    set_title("Edit Plane");

    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 20));
    box->set_margin_start(20);
    box->set_margin_end(20);
    box->set_margin_top(20);
    box->set_margin_bottom(20);
    box->set_halign(Gtk::ALIGN_CENTER);
    box->set_valign(Gtk::ALIGN_CENTER);

    auto box2 = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 20));
    auto from_rules = Gtk::manage(new Gtk::CheckButton("From rules"));
    bind_widget(from_rules, plane.from_rules);
    box2->pack_start(*from_rules, false, false, 0);

    auto net_button = Gtk::manage(new NetButton(*brd.block));
    if (plane.net) {
        net_button->set_net(plane.net->uuid);
    }
    else {
        ok_button->set_sensitive(false);
    }

    net_button->signal_changed().connect([this](const UUID &uu) {
        plane.net = brd.block->get_net(uu);
        ok_button->set_sensitive(true);
    });
    box2->pack_start(*net_button, true, true, 0);

    box->pack_start(*box2, false, false, 0);

    auto ed = Gtk::manage(new PlaneEditor(&plane.settings, &plane.priority));
    ed->set_from_rules(from_rules->get_active());
    from_rules->signal_toggled().connect([from_rules, ed] { ed->set_from_rules(from_rules->get_active()); });
    box->pack_start(*ed, true, true, 0);

    if (plane.net) {
        auto delete_button = Gtk::manage(new Gtk::Button("Delete Plane"));
        delete_button->signal_clicked().connect([this] {
            brd.planes.erase(plane.uuid);
            emit_event(ToolDataWindow::Event::OK);
        });
        box->pack_start(*delete_button, false, false, 0);
    }


    add(*box);

    box->show_all();
}

void EditPlaneWindow::on_event(ToolDataWindow::Event ev)
{
    if (ev == ToolDataWindow::Event::OK) {
        if (brd.planes.count(plane_uuid)) { // may have been deleted
            if (plane.from_rules) {
                plane.settings = brd.rules.get_plane_settings(plane.net, poly.layer);
            }
            brd.update_plane(&plane);
            brd.update_airwires(false, {plane.net->uuid});
        }
        else {
            poly.usage = nullptr;
            brd.update_planes();
        }
    }
}

} // namespace horizon