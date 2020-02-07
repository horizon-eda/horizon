#include "edit_keepout.hpp"
#include "widgets/net_button.hpp"
#include "widgets/spin_button_dim.hpp"
#include <iostream>
#include <deque>
#include <algorithm>
#include "document/idocument.hpp"
#include "common/keepout.hpp"
#include "board/board_layers.hpp"
#include "util/gtk_util.hpp"
#include "common/patch_type_names.hpp"


namespace horizon {

static const std::vector<PatchType> patch_types_cu = {PatchType::TRACK, PatchType::PAD, PatchType::PAD_TH,
                                                      PatchType::PLANE, PatchType::VIA, PatchType::HOLE_PTH};

EditKeepoutDialog::EditKeepoutDialog(Gtk::Window *parent, Keepout *k, IDocument *core, bool add_mode)
    : Gtk::Dialog("Edit Keepout", *parent, Gtk::DialogFlags::DIALOG_MODAL | Gtk::DialogFlags::DIALOG_USE_HEADER_BAR),
      keepout(k)
{
    add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
    add_button("OK", Gtk::ResponseType::RESPONSE_OK);
    set_default_response(Gtk::ResponseType::RESPONSE_OK);
    // set_default_size(400, 300);


    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 20));
    box->set_margin_start(20);
    box->set_margin_end(20);
    box->set_margin_top(20);
    box->set_margin_bottom(20);
    box->set_halign(Gtk::ALIGN_CENTER);
    box->set_valign(Gtk::ALIGN_CENTER);

    bool is_cu = BoardLayers::is_copper(keepout->polygon->layer);
    auto grid = Gtk::manage(new Gtk::Grid);
    grid->set_row_spacing(10);
    grid->set_column_spacing(10);
    int top = 0;
    {
        auto en = Gtk::manage(new Gtk::Entry);
        grid_attach_label_and_widget(grid, "Keepout class", en, top);
        bind_widget(en, keepout->keepout_class);
    }
    // TBD
    /*{
        auto sw = Gtk::manage(new Gtk::Switch);
        sw->set_halign(Gtk::ALIGN_START);
        grid_attach_label_and_widget(grid, "Exposed copper only", sw, top);
        bind_widget(sw, keepout->exposed_cu_only);
    }*/
    {
        auto sw = Gtk::manage(new Gtk::Switch);
        sw->set_halign(Gtk::ALIGN_START);
        grid_attach_label_and_widget(grid, "All copper layers", sw, top);
        if (is_cu) {
            bind_widget(sw, keepout->all_cu_layers);
        }
        else {
            sw->set_sensitive(false);
        }
    }


    box->pack_start(*grid, false, false, 0);

    auto lb = Gtk::manage(new Gtk::ListBox);
    lb->set_header_func(&header_func_separator);
    lb->set_selection_mode(Gtk::SELECTION_NONE);
    for (const auto pt : patch_types_cu) {
        auto cb = Gtk::manage(new Gtk::CheckButton(patch_type_names.at(pt)));
        if (is_cu) {
            cb->set_active(keepout->patch_types_cu.count(pt));
            cb->signal_toggled().connect([this, cb, pt] {
                if (cb->get_active()) {
                    keepout->patch_types_cu.insert(pt);
                }
                else {
                    keepout->patch_types_cu.erase(pt);
                }
            });
        }
        cb->property_margin() = 3;
        lb->append(*cb);
    }

    auto sc = Gtk::manage(new Gtk::ScrolledWindow);
    sc->set_shadow_type(Gtk::SHADOW_IN);
    sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_NEVER);
    sc->add(*lb);
    box->pack_start(*sc, false, false, 0);
    sc->set_sensitive(is_cu);

    if (!add_mode) {
        auto delete_button = Gtk::manage(new Gtk::Button("Delete Keepout"));
        delete_button->signal_clicked().connect([this, core] {
            core->delete_keepout(keepout->uuid);
            response(Gtk::RESPONSE_OK);
        });
        box->pack_start(*delete_button, false, false, 0);
    }


    get_content_area()->pack_start(*box, true, true, 0);

    show_all();
}
} // namespace horizon
