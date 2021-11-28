#include "rule_match_component_editor.hpp"
#include "block/block.hpp"
#include "document/idocument.hpp"
#include "rules/rule_match_component.hpp"
#include "widgets/component_button.hpp"
#include "widgets/pool_browser_button.hpp"

namespace horizon {
RuleMatchComponentEditor::RuleMatchComponentEditor(RuleMatchComponent &ma, class IDocument &c)
    : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 4), match(ma), core(c)
{
    combo_mode = Gtk::manage(new Gtk::ComboBoxText());
    combo_mode->append(std::to_string(static_cast<int>(RuleMatchComponent::Mode::COMPONENT)), "Component");
    combo_mode->append(std::to_string(static_cast<int>(RuleMatchComponent::Mode::PART)), "Part");
    combo_mode->set_active_id(std::to_string(static_cast<int>(match.mode)));
    combo_mode->signal_changed().connect([this] {
        match.mode = static_cast<RuleMatchComponent::Mode>(std::stoi(combo_mode->get_active_id()));
        sel_stack->set_visible_child(combo_mode->get_active_id());
        s_signal_updated.emit();
    });

    pack_start(*combo_mode, true, true, 0);
    combo_mode->show();

    sel_stack = Gtk::manage(new Gtk::Stack());
    sel_stack->set_homogeneous(true);

    component_button = Gtk::manage(new ComponentButton(core.get_top_block()));
    component_button->set_component(match.component);
    component_button->signal_changed().connect([this](const UUID &uu) {
        match.component = uu;
        s_signal_updated.emit();
    });
    sel_stack->add(*component_button, std::to_string(static_cast<int>(RuleMatchComponent::Mode::COMPONENT)));

    part_button = Gtk::manage(new PoolBrowserButton(ObjectType::PART, core.get_pool()));
    part_button->property_selected_uuid().set_value(match.part);
    part_button->property_selected_uuid().signal_changed().connect([this] {
        match.part = part_button->property_selected_uuid().get_value();
        s_signal_updated.emit();
    });
    sel_stack->add(*part_button, std::to_string(static_cast<int>(RuleMatchComponent::Mode::PART)));

    pack_start(*sel_stack, true, true, 0);
    sel_stack->show_all();

    sel_stack->set_visible_child(std::to_string(static_cast<int>(match.mode)));
}
} // namespace horizon
