#include "rule_editor_thermals.hpp"
#include "board/rule_thermals.hpp"
#include "rule_match_component_editor.hpp"
#include "rule_match_editor.hpp"
#include "util/gtk_util.hpp"
#include "widgets/spin_button_dim.hpp"
#include "widgets/multi_pad_button.hpp"
#include "document/idocument.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include "block/block.hpp"
#include "pool/part.hpp"
#include "pool/ipool.hpp"
#include "widgets/layer_combo_box.hpp"
#include "widgets/spin_button_angle.hpp"

namespace horizon {

class MatchPadsEditor : public Gtk::Box {
public:
    MatchPadsEditor(RuleThermals &r);
    typedef sigc::signal<void> type_signal_updated;
    type_signal_updated signal_updated()
    {
        return s_signal_updated;
    }
    void set_package(const Package &pkg);
    void set_can_match_pads(bool m);

private:
    Gtk::ComboBoxText *combo_mode = nullptr;
    Gtk::Stack *sel_stack = nullptr;
    class MultiPadButton *multi_pad_button = nullptr;
    RuleThermals &rule;
    type_signal_updated s_signal_updated;
};

void MatchPadsEditor::set_package(const Package &pkg)
{
    multi_pad_button->set_package(pkg);
}
void MatchPadsEditor::set_can_match_pads(bool m)
{
    combo_mode->set_sensitive(m);
    if (!m)
        combo_mode->set_active_id(std::to_string(static_cast<int>(RuleThermals::PadMode::ALL)));
}

MatchPadsEditor::MatchPadsEditor(RuleThermals &r) : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 4), rule(r)
{
    combo_mode = Gtk::manage(new Gtk::ComboBoxText());
    combo_mode->append(std::to_string(static_cast<int>(RuleThermals::PadMode::ALL)), "All pads");
    combo_mode->append(std::to_string(static_cast<int>(RuleThermals::PadMode::PADS)), "Multiple pads");
    combo_mode->set_active_id(std::to_string(static_cast<int>(rule.pad_mode)));
    combo_mode->signal_changed().connect([this] {
        rule.pad_mode = static_cast<RuleThermals::PadMode>(std::stoi(combo_mode->get_active_id()));
        sel_stack->set_visible_child(combo_mode->get_active_id());
        s_signal_updated.emit();
    });

    pack_start(*combo_mode, false, false, 0);
    combo_mode->show();

    sel_stack = Gtk::manage(new Gtk::Stack());
    sel_stack->set_homogeneous(true);

    auto *dummy_label = Gtk::manage(new Gtk::Label("matches all pads"));
    sel_stack->add(*dummy_label, std::to_string(static_cast<int>(RuleThermals::PadMode::ALL)));

    multi_pad_button = Gtk::manage(new MultiPadButton());
    multi_pad_button->set_items(rule.pads);
    multi_pad_button->signal_changed().connect([this] {
        rule.pads = multi_pad_button->get_items();
        s_signal_updated.emit();
    });
    sel_stack->add(*multi_pad_button, std::to_string(static_cast<int>(RuleThermals::PadMode::PADS)));

    pack_start(*sel_stack, true, true, 0);
    sel_stack->show_all();

    sel_stack->set_visible_child(std::to_string(static_cast<int>(rule.pad_mode)));
}

void RuleEditorThermals::populate()
{
    rule2 = &dynamic_cast<RuleThermals &>(rule);

    auto editor = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 20));
    editor->set_margin_start(20);
    editor->set_margin_end(20);
    editor->set_margin_bottom(20);

    {
        auto grid = Gtk::manage(new Gtk::Grid());
        grid->set_row_spacing(10);
        grid->set_column_spacing(20);

        {
            auto la = Gtk::manage(new Gtk::Label("Match Package"));
            la->get_style_context()->add_class("dim-label");
            la->set_halign(Gtk::ALIGN_START);
            grid->attach(*la, 0, 0, 1, 1);
        }
        {
            auto la = Gtk::manage(new Gtk::Label("Match Pads"));
            la->get_style_context()->add_class("dim-label");
            la->set_halign(Gtk::ALIGN_START);
            grid->attach(*la, 1, 0, 1, 1);
        }
        {
            auto la = Gtk::manage(new Gtk::Label("Match Net"));
            la->get_style_context()->add_class("dim-label");
            la->set_halign(Gtk::ALIGN_START);
            grid->attach(*la, 2, 0, 1, 1);
        }
        {
            auto la = Gtk::manage(new Gtk::Label("Layer"));
            la->get_style_context()->add_class("dim-label");
            la->set_halign(Gtk::ALIGN_START);
            grid->attach(*la, 3, 0, 1, 1);
        }

        auto match_component_editor = Gtk::manage(new RuleMatchComponentEditor(rule2->match_component, core));
        match_component_editor->set_hexpand(true);
        match_component_editor->set_orientation(Gtk::ORIENTATION_HORIZONTAL);
        match_component_editor->signal_updated().connect([this] {
            update_pads();
            s_signal_updated.emit();
        });
        grid->attach(*match_component_editor, 0, 1, 1, 1);


        match_pads_editor = Gtk::manage(new MatchPadsEditor(*rule2));
        match_pads_editor->set_hexpand(true);
        match_pads_editor->set_orientation(Gtk::ORIENTATION_HORIZONTAL);
        match_pads_editor->signal_updated().connect([this] { s_signal_updated.emit(); });
        grid->attach(*match_pads_editor, 1, 1, 1, 1);

        update_pads();

        auto match_editor = Gtk::manage(new RuleMatchEditor(rule2->match, core));
        match_editor->set_hexpand(true);
        match_editor->set_orientation(Gtk::ORIENTATION_HORIZONTAL);
        match_editor->signal_updated().connect([this] { s_signal_updated.emit(); });
        grid->attach(*match_editor, 2, 1, 1, 1);

        auto layer_combo = create_layer_combo(rule2->layer, true);
        layer_combo->set_hexpand(false);
        grid->attach(*layer_combo, 3, 1, 1, 1);

        grid->show_all();
        editor->pack_start(*grid, false, false, 0);
    }

    {
        auto grid = Gtk::manage(new Gtk::Grid());
        grid->set_row_spacing(10);
        grid->set_column_spacing(10);
        int top = 0;

        {
            auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
            box->get_style_context()->add_class("linked");
            auto b1 = Gtk::manage(new Gtk::RadioButton("From plane"));
            b1->set_mode(false);
            box->pack_start(*b1, true, true, 0);

            auto b2 = Gtk::manage(new Gtk::RadioButton("Solid"));
            b2->set_mode(false);
            b2->join_group(*b1);
            box->pack_start(*b2, true, true, 0);

            auto b3 = Gtk::manage(new Gtk::RadioButton("Thermal rlief"));
            b3->set_mode(false);
            b3->join_group(*b1);
            box->pack_start(*b3, true, true, 0);

            std::map<ThermalSettings::ConnectStyle, Gtk::RadioButton *> style_widgets = {
                    {ThermalSettings::ConnectStyle::FROM_PLANE, b1},
                    {ThermalSettings::ConnectStyle::SOLID, b2},
                    {ThermalSettings::ConnectStyle::THERMAL, b3},
            };

            bind_widget<ThermalSettings::ConnectStyle>(style_widgets, rule2->thermal_settings.connect_style,
                                                       [this](auto v) {
                                                           update_thermal();
                                                           s_signal_updated.emit();
                                                       });


            auto la = grid_attach_label_and_widget(grid, "Connect style", box, top);
            la->set_width_chars(12);
        }
        {
            auto sp = Gtk::manage(new SpinButtonDim());
            sp->set_range(0, 10_mm);
            bind_widget(sp, rule2->thermal_settings.thermal_gap_width);
            sp->signal_changed().connect([this] { s_signal_updated.emit(); });
            auto la = grid_attach_label_and_widget(grid, "Th. gap", sp, top);
            widgets_thermal_only.insert(la);
            widgets_thermal_only.insert(sp);
        }
        {
            auto sp = Gtk::manage(new SpinButtonDim());
            sp->set_range(0, 10_mm);
            bind_widget(sp, rule2->thermal_settings.thermal_spoke_width);
            sp->signal_changed().connect([this] { s_signal_updated.emit(); });
            auto la = grid_attach_label_and_widget(grid, "Th. spoke width", sp, top);
            widgets_thermal_only.insert(la);
            widgets_thermal_only.insert(sp);
        }
        {
            auto sp = Gtk::manage(new Gtk::SpinButton());
            sp->set_increments(1, 1);
            sp->set_range(1, 8);
            sp->set_value(rule2->thermal_settings.n_spokes);
            sp->signal_changed().connect([this, sp] {
                rule2->thermal_settings.n_spokes = sp->get_value_as_int();
                s_signal_updated.emit();
            });
            auto la = grid_attach_label_and_widget(grid, "Th. spokes", sp, top);
            widgets_thermal_only.insert(la);
            widgets_thermal_only.insert(sp);
        }
        {
            auto sp = Gtk::manage(new SpinButtonAngle());
            sp->set_increments(65536 / 8, 65536 / 8);
            sp->set_value(rule2->thermal_settings.angle);
            sp->signal_changed().connect([this, sp] {
                rule2->thermal_settings.angle = sp->get_value_as_int();
                s_signal_updated.emit();
            });
            auto la = grid_attach_label_and_widget(grid, "Th. spoke angle", sp, top);
            widgets_thermal_only.insert(la);
            widgets_thermal_only.insert(sp);
        }
        for (auto &it : widgets_thermal_only) {
            it->set_no_show_all();
            it->show();
        }
        update_thermal();

        grid->show_all();
        editor->pack_start(*grid, false, false, 0);
    }

    pack_start(*editor, true, true, 0);
    editor->show();
}

void RuleEditorThermals::update_thermal()
{
    for (auto &it : widgets_thermal_only) {
        it->set_visible(rule2->thermal_settings.connect_style == ThermalSettings::ConnectStyle::THERMAL);
    }
}

void RuleEditorThermals::update_pads()
{
    const Package *pkg = nullptr;

    switch (rule2->match_component.mode) {
    case RuleMatchComponent::Mode::COMPONENT: {
        if (rule2->match_component.component) {
            const auto &comp = core.get_top_block()->components.at(rule2->match_component.component);
            if (comp.part)
                pkg = comp.part->package;
            for (const auto &[uu, bpkg] : dynamic_cast<IDocumentBoard &>(core).get_board()->packages) {
                if (bpkg.component->uuid == comp.uuid) {
                    pkg = &bpkg.package;
                    break;
                }
            }
        }
    } break;

    case RuleMatchComponent::Mode::COMPONENTS: {
        std::set<UUID> pkgs;
        for (const auto &[uu, bpkg] : dynamic_cast<IDocumentBoard &>(core).get_board()->packages) {
            if (rule2->match_component.components.count(bpkg.component->uuid))
                pkgs.insert(bpkg.package.uuid);
        }
        if (pkgs.size() == 1) {
            pkg = core.get_pool().get_package(*pkgs.begin());
        }
    } break;

    case RuleMatchComponent::Mode::PART: {
        pkg = core.get_pool().get_part(rule2->match_component.part)->package;
    } break;
    }

    if (pkg) {
        match_pads_editor->set_can_match_pads(true);
        match_pads_editor->set_package(*pkg);
    }
    else {
        match_pads_editor->set_can_match_pads(false);
    }
}

} // namespace horizon
