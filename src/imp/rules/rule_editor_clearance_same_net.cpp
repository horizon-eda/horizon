#include "rule_editor_clearance_same_net.hpp"
#include "board/rule_clearance_same_net.hpp"
#include "common/patch_type_names.hpp"
#include "document/idocument.hpp"
#include "rule_match_editor.hpp"
#include "widgets/chooser_buttons.hpp"
#include "widgets/parameter_set_editor.hpp"
#include "widgets/spin_button_dim.hpp"
#include "dialogs/dialogs.hpp"

namespace horizon {

static const std::vector<PatchType> patch_types = {PatchType::PAD, PatchType::PAD_TH, PatchType::VIA,
                                                   PatchType::HOLE_NPTH};

void RuleEditorClearanceSameNet::populate()
{
    rule2 = dynamic_cast<RuleClearanceSameNet *>(rule);

    auto editor = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 20));
    editor->set_margin_start(20);
    editor->set_margin_end(20);
    editor->set_margin_bottom(20);

    auto grid = Gtk::manage(new Gtk::Grid());
    grid->set_row_spacing(10);
    grid->set_column_spacing(20);

    {
        auto la = Gtk::manage(new Gtk::Label("Match"));
        la->get_style_context()->add_class("dim-label");
        la->set_halign(Gtk::ALIGN_START);
        grid->attach(*la, 0, 0, 1, 1);
    }
    {
        auto la = Gtk::manage(new Gtk::Label("Layer"));
        la->get_style_context()->add_class("dim-label");
        la->set_halign(Gtk::ALIGN_START);
        grid->attach(*la, 1, 0, 1, 1);
    }


    auto match_editor = Gtk::manage(new RuleMatchEditor(&rule2->match, core));
    match_editor->set_orientation(Gtk::ORIENTATION_HORIZONTAL);
    match_editor->signal_updated().connect([this] { s_signal_updated.emit(); });
    match_editor->set_hexpand(true);
    grid->attach(*match_editor, 0, 1, 1, 1);

    auto *layer_combo = Gtk::manage(new Gtk::ComboBoxText());
    for (const auto &it : core->get_layer_provider()->get_layers()) {
        if (it.second.copper)
            layer_combo->insert(0, std::to_string(it.first), it.second.name + ": " + std::to_string(it.first));
    }
    layer_combo->insert(0, "10000", "Any layer");
    layer_combo->set_hexpand(true);
    layer_combo->set_active_id(std::to_string(rule2->layer));
    layer_combo->signal_changed().connect([this, layer_combo] {
        rule2->layer = std::stoi(layer_combo->get_active_id());
        s_signal_updated.emit();
    });
    grid->attach(*layer_combo, 1, 1, 1, 1);


    grid->show_all();
    editor->pack_start(*grid, false, false, 0);

    auto clearance_grid = Gtk::manage(new Gtk::Grid);
    clearance_grid->set_row_spacing(10);
    clearance_grid->set_column_spacing(10);

    {
        int left = 1;
        for (const auto it : patch_types) {
            if (it == PatchType::HOLE_NPTH)
                continue;
            auto *box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 5));

            auto *bu = Gtk::manage(new Gtk::Button());
            bu->set_image_from_icon_name("pan-down-symbolic", Gtk::ICON_SIZE_BUTTON);
            bu->get_style_context()->add_class("imp-rule-editor-tiny-button-column");
            bu->set_tooltip_text("Set column");
            bu->signal_clicked().connect([this, left] { set_some(-1, left); });
            box->pack_start(*bu, false, false, 0);

            auto *la = Gtk::manage(new Gtk::Label(patch_type_names.at(it)));
            la->set_xalign(0);
            box->pack_start(*la, true, true, 0);

            clearance_grid->attach(*box, left, 0, 1, 1);
            left++;
        }
        int top = 1;
        for (const auto it : patch_types) {
            if (it == PatchType::PAD)
                continue;
            auto *box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 5));
            auto *la = Gtk::manage(new Gtk::Label(patch_type_names.at(it)));
            la->set_xalign(1);
            box->pack_start(*la, true, true, 0);

            auto *bu = Gtk::manage(new Gtk::Button());
            bu->set_image_from_icon_name("pan-end-symbolic", Gtk::ICON_SIZE_BUTTON);
            bu->get_style_context()->add_class("imp-rule-editor-tiny-button-row");
            bu->set_tooltip_text("Set row");
            bu->signal_clicked().connect([this, top] { set_some(top, -1); });
            box->pack_start(*bu, false, false, 0);

            clearance_grid->attach(*box, 0, top, 1, 1);
            top++;
        }
    }
    {
        auto *bu = Gtk::manage(new Gtk::Button("set all"));
        bu->get_style_context()->add_class("imp-rule-editor-tiny-button-column");
        bu->signal_clicked().connect([this] { set_some(-1, -1); });
        clearance_grid->attach(*bu, 0, 0, 1, 1);
    }

    {
        int left = 1;
        for (const auto it_col : patch_types) {
            int top = 1;
            for (const auto it_row : patch_types) {
                if (left < top) {
                    auto *sp = Gtk::manage(new SpinButtonDim());
                    sp->set_range(0, 100_mm);
                    sp->set_width_chars(8);
                    auto cl = rule2->get_clearance(it_col, it_row);
                    if (cl >= 0) {
                        sp->set_value(cl);
                    }
                    sp->signal_value_changed().connect([this, sp, it_row, it_col] {
                        rule2->set_clearance(it_col, it_row, sp->get_value_as_int());
                        s_signal_updated.emit();
                    });
                    auto cb = Gtk::manage(new Gtk::CheckButton);
                    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 5));
                    box->pack_start(*cb, false, false, 0);
                    box->pack_start(*sp, true, true, 0);
                    cb->set_active(cl >= 0);
                    sp->set_sensitive(cl >= 0);
                    cb->signal_toggled().connect([this, cb, sp, it_row, it_col] {
                        const bool act = cb->get_active();
                        sp->set_sensitive(act);
                        if (act) {
                            rule2->set_clearance(it_col, it_row, sp->get_value_as_int());
                        }
                        else {
                            rule2->set_clearance(it_col, it_row, -1);
                        }
                        s_signal_updated.emit();
                    });

                    clearance_grid->attach(*box, left, top - 1, 1, 1);
                    spin_buttons.emplace(std::piecewise_construct, std::forward_as_tuple(top - 1, left),
                                         std::forward_as_tuple(sp));
                }
                top++;
            }
            left++;
        }
    }

    clearance_grid->show_all();
    editor->pack_start(*clearance_grid, false, false, 0);

    pack_start(*editor, true, true, 0);
    editor->show();
}

void RuleEditorClearanceSameNet::set_some(int row, int col)
{
    Dialogs dias;
    dias.set_parent(dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW)));
    auto res = dias.ask_datum("Enter clearance", 0.1_mm);
    if (!res.first)
        return;
    for (auto &[pos, sp] : spin_buttons) {
        if ((pos.first == row || row == -1) && (pos.second == col || col == -1)) {
            if (sp->get_sensitive())
                sp->set_value(res.second);
        }
    }
}
} // namespace horizon
