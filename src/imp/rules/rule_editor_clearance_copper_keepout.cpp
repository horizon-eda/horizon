#include "rule_editor_clearance_copper_keepout.hpp"
#include "board/rule_clearance_copper_keepout.hpp"
#include "common/patch_type_names.hpp"
#include "rule_match_editor.hpp"
#include "rule_match_keepout_editor.hpp"
#include "widgets/chooser_buttons.hpp"
#include "widgets/parameter_set_editor.hpp"
#include "widgets/spin_button_dim.hpp"

namespace horizon {

static const std::vector<PatchType> patch_types_cu = {PatchType::TRACK, PatchType::PAD, PatchType::PAD_TH,
                                                      PatchType::PLANE, PatchType::VIA, PatchType::HOLE_PTH};


void RuleEditorClearanceCopperKeepout::populate()
{
    rule2 = dynamic_cast<RuleClearanceCopperKeepout *>(rule);

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
        auto la = Gtk::manage(new Gtk::Label("Keepout Match"));
        la->get_style_context()->add_class("dim-label");
        la->set_halign(Gtk::ALIGN_START);
        grid->attach(*la, 1, 0, 1, 1);
    }
    {
        auto la = Gtk::manage(new Gtk::Label("Routing offset"));
        la->get_style_context()->add_class("dim-label");
        la->set_halign(Gtk::ALIGN_START);
        grid->attach(*la, 2, 0, 1, 1);
    }

    auto match_editor = Gtk::manage(new RuleMatchEditor(&rule2->match, core));
    match_editor->set_orientation(Gtk::ORIENTATION_HORIZONTAL);
    match_editor->signal_updated().connect([this] { s_signal_updated.emit(); });
    match_editor->set_hexpand(true);
    grid->attach(*match_editor, 0, 1, 1, 1);

    auto match_keepout_editor = Gtk::manage(new RuleMatchKeepoutEditor(&rule2->match_keepout, core));
    match_keepout_editor->set_orientation(Gtk::ORIENTATION_HORIZONTAL);
    match_keepout_editor->signal_updated().connect([this] { s_signal_updated.emit(); });
    match_keepout_editor->set_hexpand(true);
    grid->attach(*match_keepout_editor, 1, 1, 1, 1);


    auto sp_routing_offset = Gtk::manage(new SpinButtonDim);
    sp_routing_offset->set_range(0, 10_mm);
    sp_routing_offset->set_value(rule2->routing_offset);
    sp_routing_offset->signal_value_changed().connect([this, sp_routing_offset] {
        rule2->routing_offset = sp_routing_offset->get_value_as_int();
        s_signal_updated.emit();
    });
    grid->attach(*sp_routing_offset, 2, 1, 1, 1);

    grid->show_all();
    editor->pack_start(*grid, false, false, 0);

    auto clearance_grid = Gtk::manage(new Gtk::Grid);
    clearance_grid->set_row_spacing(10);
    clearance_grid->set_column_spacing(10);

    {

        int top = 0;
        for (const auto it : patch_types_cu) {
            auto *la = Gtk::manage(new Gtk::Label(patch_type_names.at(it)));
            la->set_xalign(1);
            clearance_grid->attach(*la, 0, top, 1, 1);
            top++;
        }
    }
    {
        int top = 0;
        for (const auto it_row : patch_types_cu) {
            auto *sp = Gtk::manage(new SpinButtonDim());
            sp->set_range(0, 100_mm);
            sp->set_value(rule2->get_clearance(it_row));
            sp->signal_value_changed().connect([this, sp, it_row] {
                rule2->set_clearance(it_row, sp->get_value_as_int());
                s_signal_updated.emit();
            });
            clearance_grid->attach(*sp, 1, top, 1, 1);
            top++;
        }
    }

    clearance_grid->show_all();
    editor->pack_start(*clearance_grid, false, false, 0);

    pack_start(*editor, true, true, 0);
    editor->show();
}


} // namespace horizon
