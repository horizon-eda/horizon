#include "rule_editor_clearance_silk_exp_copper.hpp"
#include "board/rule_clearance_silk_exp_copper.hpp"
#include "board/rule_parameters.hpp"
#include "package/rule_clearance_package.hpp"
#include "widgets/spin_button_dim.hpp"

namespace horizon {

SpinButtonDim *RuleEditorClearanceSilkscreenExposedCopper::create_sp_dim(const std::string &label)
{
    auto la = Gtk::manage(new Gtk::Label(label));
    la->get_style_context()->add_class("dim-label");
    la->set_halign(Gtk::ALIGN_END);

    auto sp = Gtk::manage(new SpinButtonDim());
    sp->set_hexpand(true);
    grid->attach(*la, 0, top, 1, 1);
    grid->attach(*sp, 1, top, 1, 1);
    top++;

    return sp;
}

void RuleEditorClearanceSilkscreenExposedCopper::populate()
{
    grid = Gtk::manage(new Gtk::Grid());
    grid->set_row_spacing(10);
    grid->set_column_spacing(10);
    grid->set_margin_start(20);
    grid->set_margin_end(20);
    grid->set_margin_bottom(20);
    pack_start(*grid, true, true, 0);
    grid->set_halign(Gtk::ALIGN_START);

    if (auto rule1 = dynamic_cast<RuleClearanceSilkscreenExposedCopper *>(&rule)) {
        auto sp_top = create_sp_dim("Top clearance");
        auto sp_bot = create_sp_dim("Bottom clearance");
        sp_top->set_range(0, 100_mm);
        sp_bot->set_range(0, 100_mm);
        sp_top->set_value(rule1->clearance_top);
        sp_bot->set_value(rule1->clearance_bottom);
        sp_top->signal_value_changed().connect([this, sp_top, rule1] {
            rule1->clearance_top = sp_top->get_value_as_int();
            s_signal_updated.emit();
        });
        sp_bot->signal_value_changed().connect([this, sp_bot, rule1] {
            rule1->clearance_bottom = sp_bot->get_value_as_int();
            s_signal_updated.emit();
        });
    }
    else if (auto rule2 = dynamic_cast<RuleParameters *>(&rule)) {
        auto sp_solder = create_sp_dim("Solder mask expansion");
        auto sp_paste = create_sp_dim("Paste mask contraction");
        auto sp_courtyard = create_sp_dim("Courtyard expansion");
        auto sp_via_solder = create_sp_dim("Via solder mask expansion");
        auto sp_hole_solder = create_sp_dim("Hole solder mask expansion");
        sp_solder->set_range(0, 100_mm);
        sp_hole_solder->set_range(0, 100_mm);
        sp_via_solder->set_range(0, 100_mm);
        sp_paste->set_range(0, 100_mm);
        sp_courtyard->set_range(0, 100_mm);

        sp_solder->set_value(rule2->solder_mask_expansion);
        sp_via_solder->set_value(rule2->via_solder_mask_expansion);
        sp_hole_solder->set_value(rule2->hole_solder_mask_expansion);
        sp_paste->set_value(rule2->paste_mask_contraction);
        sp_courtyard->set_value(rule2->courtyard_expansion);

        sp_solder->signal_value_changed().connect([this, sp_solder, rule2] {
            rule2->solder_mask_expansion = sp_solder->get_value_as_int();
            s_signal_updated.emit();
        });

        sp_via_solder->signal_value_changed().connect([this, sp_via_solder, rule2] {
            rule2->via_solder_mask_expansion = sp_via_solder->get_value_as_int();
            s_signal_updated.emit();
        });

        sp_hole_solder->signal_value_changed().connect([this, sp_hole_solder, rule2] {
            rule2->hole_solder_mask_expansion = sp_hole_solder->get_value_as_int();
            s_signal_updated.emit();
        });

        sp_paste->signal_value_changed().connect([this, sp_paste, rule2] {
            rule2->paste_mask_contraction = sp_paste->get_value_as_int();
            s_signal_updated.emit();
        });

        sp_courtyard->signal_value_changed().connect([this, sp_courtyard, rule2] {
            rule2->courtyard_expansion = sp_courtyard->get_value_as_int();
            s_signal_updated.emit();
        });
    }
    else if (auto rule2 = dynamic_cast<RuleClearancePackage *>(&rule)) {
        auto sp_silk_cu = create_sp_dim("Silkscreen to copper clearance");
        auto sp_silk_pkg = create_sp_dim("Silkscreen to package clearance");

        sp_silk_cu->set_range(0, 100_mm);
        sp_silk_pkg->set_range(0, 100_mm);

        sp_silk_cu->set_value(rule2->clearance_silkscreen_cu);
        sp_silk_pkg->set_value(rule2->clearance_silkscreen_pkg);

        sp_silk_cu->signal_value_changed().connect([this, sp_silk_cu, rule2] {
            rule2->clearance_silkscreen_cu = sp_silk_cu->get_value_as_int();
            s_signal_updated.emit();
        });

        sp_silk_pkg->signal_value_changed().connect([this, sp_silk_pkg, rule2] {
            rule2->clearance_silkscreen_pkg = sp_silk_pkg->get_value_as_int();
            s_signal_updated.emit();
        });
    }

    grid->show_all();
}
} // namespace horizon
