#include "rule_match_keepout_editor.hpp"
#include "block/block.hpp"
#include "document/idocument.hpp"
#include "rules/rule_match_keepout.hpp"
#include "widgets/component_button.hpp"

namespace horizon {
RuleMatchKeepoutEditor::RuleMatchKeepoutEditor(RuleMatchKeepout *ma, class IDocument *c)
    : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 4), match(ma), core(c)
{
    combo_mode = Gtk::manage(new Gtk::ComboBoxText());
    combo_mode->append(std::to_string(static_cast<int>(RuleMatchKeepout::Mode::ALL)), "All");
    combo_mode->append(std::to_string(static_cast<int>(RuleMatchKeepout::Mode::KEEPOUT_CLASS)), "Keepout class");
    combo_mode->append(std::to_string(static_cast<int>(RuleMatchKeepout::Mode::COMPONENT)), "Component");
    combo_mode->set_active_id(std::to_string(static_cast<int>(match->mode)));
    combo_mode->signal_changed().connect([this] {
        match->mode = static_cast<RuleMatchKeepout::Mode>(std::stoi(combo_mode->get_active_id()));
        sel_stack->set_visible_child(combo_mode->get_active_id());
        s_signal_updated.emit();
    });

    pack_start(*combo_mode, true, true, 0);
    combo_mode->show();

    sel_stack = Gtk::manage(new Gtk::Stack());
    sel_stack->set_homogeneous(true);
    Block *block = core->get_block();
    assert(block);

    keepout_class_entry = Gtk::manage(new Gtk::Entry());
    keepout_class_entry->set_text(match->keepout_class);
    keepout_class_entry->signal_changed().connect([this] {
        match->keepout_class = keepout_class_entry->get_text();
        s_signal_updated.emit();
    });
    sel_stack->add(*keepout_class_entry, std::to_string(static_cast<int>(RuleMatchKeepout::Mode::KEEPOUT_CLASS)));

    component_button = Gtk::manage(new ComponentButton(block));
    component_button->signal_changed().connect([this](const UUID &uu) {
        match->component = uu;
        s_signal_updated.emit();
    });
    sel_stack->add(*component_button, std::to_string(static_cast<int>(RuleMatchKeepout::Mode::COMPONENT)));


    auto *dummy_label = Gtk::manage(new Gtk::Label("matches all keepouts"));
    sel_stack->add(*dummy_label, std::to_string(static_cast<int>(RuleMatchKeepout::Mode::ALL)));

    pack_start(*sel_stack, true, true, 0);
    sel_stack->show_all();

    sel_stack->set_visible_child(std::to_string(static_cast<int>(match->mode)));
}
} // namespace horizon
