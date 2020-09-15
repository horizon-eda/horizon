#include "edit_board_hole.hpp"
#include "widgets/parameter_set_editor.hpp"
#include "widgets/pool_browser_button.hpp"
#include "widgets/net_button.hpp"
#include "package/pad.hpp"
#include "widgets/pool_browser_padstack.hpp"
#include "board/board_hole.hpp"
#include "util/util.hpp"
#include "pool/ipool.hpp"
#include "block/block.hpp"
#include <iostream>
#include <deque>
#include <algorithm>

namespace horizon {

class MyParameterSetEditor : public ParameterSetEditor {
public:
private:
    using ParameterSetEditor::ParameterSetEditor;
    Gtk::Widget *create_extra_widget(ParameterID id) override
    {
        auto w = Gtk::manage(new Gtk::Button());
        w->set_image_from_icon_name("object-select-symbolic", Gtk::ICON_SIZE_BUTTON);
        w->set_tooltip_text("Apply to all selected holes (Shift+Enter)");
        w->signal_clicked().connect([this, id] { s_signal_apply_all.emit(id); });
        return w;
    }
};

BoardHoleDialog::BoardHoleDialog(Gtk::Window *parent, std::set<BoardHole *> &holes, IPool &p, Block &b)
    : Gtk::Dialog("Edit hole", *parent, Gtk::DialogFlags::DIALOG_MODAL | Gtk::DialogFlags::DIALOG_USE_HEADER_BAR),
      pool(p), block(b)
{
    set_default_size(300, 600);
    add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
    add_button("OK", Gtk::ResponseType::RESPONSE_OK);
    set_default_response(Gtk::ResponseType::RESPONSE_OK);

    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0));
    auto combo = Gtk::manage(new Gtk::ComboBoxText());
    combo->set_margin_start(8);
    combo->set_margin_end(8);
    combo->set_margin_top(8);
    combo->set_margin_bottom(8);
    box->pack_start(*combo, false, false, 0);

    auto grid = Gtk::manage(new Gtk::Grid());
    grid->set_row_spacing(8);
    grid->set_column_spacing(8);
    grid->set_margin_start(8);
    grid->set_margin_end(8);
    {
        auto la = Gtk::manage(new Gtk::Label("Padstack"));
        la->get_style_context()->add_class("dim-label");
        la->set_halign(Gtk::ALIGN_END);
        grid->attach(*la, 0, 0, 1, 1);
    }
    {
        auto la = Gtk::manage(new Gtk::Label("Net"));
        la->get_style_context()->add_class("dim-label");
        la->set_halign(Gtk::ALIGN_END);
        grid->attach(*la, 0, 1, 1, 1);
    }

    auto padstack_apply_all_button = Gtk::manage(new Gtk::Button);
    padstack_apply_all_button->set_image_from_icon_name("object-select-symbolic", Gtk::ICON_SIZE_BUTTON);
    padstack_apply_all_button->set_tooltip_text("Apply to all pads");
    padstack_apply_all_button->signal_clicked().connect([this, holes] {
        auto ps = pool.get_padstack(padstack_button->property_selected_uuid());
        for (auto &it : holes) {
            it->pool_padstack = ps;
            it->padstack = *ps;
        }
    });
    grid->attach(*padstack_apply_all_button, 2, 0, 1, 1);

    auto net_apply_all_button = Gtk::manage(new Gtk::Button);
    net_apply_all_button->set_image_from_icon_name("object-select-symbolic", Gtk::ICON_SIZE_BUTTON);
    net_apply_all_button->set_tooltip_text("Apply to all pads");
    net_apply_all_button->signal_clicked().connect([this, holes] {
        auto net = block.get_net(net_button->get_net());
        for (auto &it : holes) {
            if (it->pool_padstack->type == Padstack::Type::HOLE)
                it->net = net;
        }
    });
    grid->attach(*net_apply_all_button, 2, 1, 1, 1);

    box->pack_start(*grid, false, false, 0);

    std::map<UUID, BoardHole *> holemap;

    for (auto it : holes) {
        combo->append(static_cast<std::string>(it->uuid), coord_to_string(it->placement.shift));
        holemap.emplace(it->uuid, it);
    }

    combo->signal_changed().connect([this, combo, holemap, box, grid, holes] {
        UUID uu(combo->get_active_id());
        auto hole = holemap.at(uu);
        if (editor)
            delete editor;
        editor = Gtk::manage(new MyParameterSetEditor(&hole->parameter_set, false));
        editor->populate();
        editor->signal_apply_all().connect([holes, hole](ParameterID id) {
            for (auto it : holes) {
                it->parameter_set[id] = hole->parameter_set.at(id);
            }
        });
        editor->signal_activate_last().connect([this] { response(Gtk::RESPONSE_OK); });
        box->pack_start(*editor, true, true, 0);
        editor->show();

        if (padstack_button)
            delete padstack_button;

        padstack_button = Gtk::manage(new PoolBrowserButton(ObjectType::PADSTACK, pool));
        auto &br = dynamic_cast<PoolBrowserPadstack &>(padstack_button->get_browser());
        br.set_include_padstack_type(Padstack::Type::MECHANICAL, true);
        br.set_include_padstack_type(Padstack::Type::HOLE, true);
        br.set_include_padstack_type(Padstack::Type::TOP, false);
        br.set_include_padstack_type(Padstack::Type::THROUGH, false);
        br.set_include_padstack_type(Padstack::Type::BOTTOM, false);
        padstack_button->property_selected_uuid() = hole->pool_padstack->uuid;
        padstack_button->property_selected_uuid().signal_changed().connect([this, hole] {
            auto ps = pool.get_padstack(padstack_button->property_selected_uuid());
            hole->pool_padstack = ps;
            hole->padstack = *ps;
            net_button->set_sensitive(ps->type == Padstack::Type::HOLE);
            if (ps->type == Padstack::Type::MECHANICAL) {
                net_button->set_net(UUID());
                hole->net = nullptr;
            }
        });
        padstack_button->set_hexpand(true);
        grid->attach(*padstack_button, 1, 0, 1, 1);
        padstack_button->show();

        if (net_button)
            delete net_button;
        net_button = Gtk::manage(new NetButton(block));

        if (hole->pool_padstack->type == Padstack::Type::HOLE)
            net_button->set_net(hole->net ? hole->net->uuid : UUID());
        else
            net_button->set_sensitive(false);

        net_button->signal_changed().connect([this, hole](const UUID &net_uu) { hole->net = block.get_net(net_uu); });
        grid->attach(*net_button, 1, 1, 1, 1);
        net_button->show();
    });

    combo->set_active(0);
    editor->focus_first();

    get_content_area()->pack_start(*box, true, true, 0);
    get_content_area()->set_border_width(0);


    show_all();
}
} // namespace horizon
