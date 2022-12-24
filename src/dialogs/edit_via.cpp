#include "edit_via.hpp"
#include "widgets/parameter_set_editor.hpp"
#include "board/via.hpp"
#include "pool/ipool.hpp"
#include "util/geom_util.hpp"
#include "block/net.hpp"
#include <iostream>
#include <deque>
#include <algorithm>
#include "widgets/pool_browser_button.hpp"
#include "widgets/pool_browser_padstack.hpp"

namespace horizon {

EditViaDialog::EditViaDialog(Gtk::Window *parent, std::set<Via *> &vias, IPool &pool, IPool &pool_caching)
    : Gtk::Dialog("Edit via", *parent, Gtk::DialogFlags::DIALOG_MODAL | Gtk::DialogFlags::DIALOG_USE_HEADER_BAR)
{
    set_default_size(400, 300);
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

    auto padstack_apply_all_button = Gtk::manage(new Gtk::Button);
    padstack_apply_all_button->set_image_from_icon_name("object-select-symbolic", Gtk::ICON_SIZE_BUTTON);
    padstack_apply_all_button->set_tooltip_text("Apply to all selected vias");
    padstack_apply_all_button->signal_clicked().connect([this, vias, &pool_caching] {
        for (auto &via : vias) {
            if (via->source == Via::Source::LOCAL) {
                via->pool_padstack = pool_caching.get_padstack(button_vp->property_selected_uuid());
            }
        }
    });
    grid->attach(*padstack_apply_all_button, 2, 0, 1, 1);

    auto rules_apply_all_button = Gtk::manage(new Gtk::Button);
    rules_apply_all_button->set_image_from_icon_name("object-select-symbolic", Gtk::ICON_SIZE_BUTTON);
    rules_apply_all_button->set_tooltip_text("Apply to all selected vias");
    rules_apply_all_button->signal_clicked().connect([this, vias] {
        for (auto &via : vias) {
            if (cb_from_rules->get_active())
                via->source = Via::Source::RULES;
            else
                via->source = Via::Source::LOCAL;
            update_sensitive();
        }
    });
    grid->attach(*rules_apply_all_button, 2, 1, 1, 1);

    box->pack_start(*grid, false, false, 0);

    std::map<UUID, Via *> viamap;

    for (auto via : vias) {
        std::string name;
        if (via->junction->net) {
            name = via->junction->net->name;
        }
        else {
            name = "<no net>";
        }
        combo->append(static_cast<std::string>(via->uuid), coord_to_string(via->junction->position) + " " + name);
        viamap.emplace(via->uuid, via);
    }

    combo->signal_changed().connect([this, combo, viamap, box, grid, vias, &pool, &pool_caching] {
        UUID uu(combo->get_active_id());
        auto via = viamap.at(uu);
        if (editor)
            delete editor;
        editor = Gtk::manage(new ParameterSetEditor(&via->parameter_set, false));
        editor->set_has_apply_all("Apply to all selected vias (Shift+Enter)");
        editor->populate();
        editor->signal_apply_all().connect([vias, via](ParameterID id) {
            for (auto it : vias) {
                it->parameter_set[id] = via->parameter_set.at(id);
            }
        });
        editor->signal_activate_last().connect([this] { response(Gtk::RESPONSE_OK); });
        box->pack_start(*editor, true, true, 0);
        editor->show();

        if (button_vp)
            delete button_vp;

        button_vp = Gtk::manage(new PoolBrowserButton(ObjectType::PADSTACK, pool));
        {
            auto &br = dynamic_cast<PoolBrowserPadstack &>(button_vp->get_browser());
            br.set_padstacks_included({Padstack::Type::VIA});
        }
        button_vp->property_selected_uuid() = via->pool_padstack->uuid;
        button_vp->property_selected_uuid().signal_changed().connect([this, via, &pool_caching] {
            if (via->source == Via::Source::LOCAL)
                via->pool_padstack = pool_caching.get_padstack(button_vp->property_selected_uuid());
        });
        button_vp->set_hexpand(true);
        grid->attach(*button_vp, 1, 0, 1, 1);
        button_vp->show();

        if (cb_from_rules)
            delete cb_from_rules;
        cb_from_rules = Gtk::manage(new Gtk::CheckButton("From rules"));
        cb_from_rules->set_active(via->source == Via::Source::RULES);
        cb_from_rules->signal_toggled().connect([this, via] {
            if (cb_from_rules->get_active())
                via->source = Via::Source::RULES;
            else
                via->source = Via::Source::LOCAL;
            update_sensitive();
        });
        grid->attach(*cb_from_rules, 1, 1, 1, 1);
        cb_from_rules->show();

        update_sensitive();
    });

    combo->set_active(0);
    editor->focus_first();
    update_sensitive();

    get_content_area()->pack_start(*box, true, true, 0);
    get_content_area()->set_border_width(0);

    show_all();
}

void EditViaDialog::update_sensitive()
{
    auto s = !cb_from_rules->get_active();
    button_vp->set_sensitive(s);
    editor->set_sensitive(s);
}
} // namespace horizon
