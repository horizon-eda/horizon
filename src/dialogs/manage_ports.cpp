#include "manage_ports.hpp"
#include "block/block.hpp"
#include "select_net.hpp"
#include "util/gtk_util.hpp"
#include "widgets/net_selector.hpp"
#include <iostream>
#include <deque>
#include <algorithm>

namespace horizon {

class PortEditor : public Gtk::Box {
public:
    PortEditor(Net &n, Block &bl) : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4), net(n), block(bl)
    {
        auto la = Gtk::manage(new Gtk::Label());
        la->set_xalign(0);
        la->set_text(net.name);
        pack_start(*la, true, true, 0);

        auto dir_combo = Gtk::manage(new Gtk::ComboBoxText);
        for (const auto &[dir, name] : Pin::direction_names) {
            dir_combo->append(std::to_string(static_cast<int>(dir)), name);
        }
        dir_combo->set_active_id(std::to_string(static_cast<int>(net.port_direction)));
        dir_combo->signal_changed().connect([this, dir_combo] {
            net.port_direction = static_cast<Pin::Direction>(std::stoi(dir_combo->get_active_id()));
        });
        pack_start(*dir_combo, false, false, 0);


        delete_button = Gtk::manage(new Gtk::Button());
        delete_button->set_image_from_icon_name("list-remove-symbolic", Gtk::ICON_SIZE_BUTTON);
        pack_start(*delete_button, false, false, 0);
        delete_button->signal_clicked().connect([this] {
            net.is_port = false;
            delete this->get_parent();
        });

        set_margin_start(8);
        set_margin_end(8);
        set_margin_top(4);
        set_margin_bottom(4);
        show_all();
    }
    Gtk::Button *delete_button = nullptr;

private:
    Net &net;
    Block &block;
};

ManagePortsDialog::ManagePortsDialog(Gtk::Window *parent, Block &bl)
    : Gtk::Dialog("Manage ports", *parent, Gtk::DialogFlags::DIALOG_MODAL | Gtk::DialogFlags::DIALOG_USE_HEADER_BAR),
      block(bl)
{
    add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
    add_button("OK", Gtk::ResponseType::RESPONSE_OK);
    set_default_response(Gtk::ResponseType::RESPONSE_OK);
    set_default_size(400, 300);


    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    auto add_button = Gtk::manage(new Gtk::Button("Add port"));
    add_button->signal_clicked().connect(sigc::mem_fun(this, &ManagePortsDialog::handle_add_port));
    add_button->set_halign(Gtk::ALIGN_START);
    add_button->set_margin_bottom(8);
    add_button->set_margin_top(8);
    add_button->set_margin_start(8);
    add_button->set_margin_end(8);

    box->pack_start(*add_button, false, false, 0);

    {
        auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
        box->pack_start(*sep, false, false, 0);
    }


    auto sc = Gtk::manage(new Gtk::ScrolledWindow());
    listbox = Gtk::manage(new Gtk::ListBox());
    listbox->set_selection_mode(Gtk::SELECTION_NONE);
    listbox->set_header_func(&header_func_separator);
    sc->add(*listbox);
    box->pack_start(*sc, true, true, 0);

    for (auto &[uu, net] : block.nets) {
        if (net.is_port) {
            auto ed = Gtk::manage(new PortEditor(net, block));
            listbox->add(*ed);
        }
    }

    get_content_area()->pack_start(*box, true, true, 0);
    get_content_area()->set_border_width(0);
    show_all();
}

void ManagePortsDialog::handle_add_port()
{
    SelectNetDialog dia(this, block, "Select Net");
    if (dia.run() == Gtk::RESPONSE_OK) {
        auto net = block.get_net(dia.net_selector->get_selected_net());
        if (net->is_port == false) {
            net->is_port = true;
            auto ed = Gtk::manage(new PortEditor(*net, block));
            listbox->add(*ed);
        }
    }
}
} // namespace horizon
