#include "rule_editor_hole_size.hpp"
#include "board/rule_hole_size.hpp"
#include "rule_match_editor.hpp"
#include "widgets/spin_button_dim.hpp"

namespace horizon {
void RuleEditorHoleSize::populate()
{
    rule2 = dynamic_cast<RuleHoleSize *>(rule);

    builder = Gtk::Builder::create_from_resource("/org/horizon-eda/horizon/imp/rules/rule_editor_hole_size.ui");
    Gtk::Box *editor;
    builder->get_widget("editor", editor);
    pack_start(*editor, true, true, 0);
    auto sp_min = create_spinbutton("min_dia_box");
    auto sp_max = create_spinbutton("max_dia_box");
    sp_min->set_range(0, 100_mm);
    sp_max->set_range(0, 100_mm);
    sp_min->set_value(rule2->diameter_min);
    sp_max->set_value(rule2->diameter_max);
    sp_min->signal_value_changed().connect([this, sp_min] {
        rule2->diameter_min = sp_min->get_value_as_int();
        s_signal_updated.emit();
    });
    sp_max->signal_value_changed().connect([this, sp_max] {
        rule2->diameter_max = sp_max->get_value_as_int();
        s_signal_updated.emit();
    });

    auto match_editor = create_rule_match_editor("match_box", &rule2->match);
    match_editor->set_orientation(Gtk::ORIENTATION_HORIZONTAL);
    match_editor->signal_updated().connect([this] { s_signal_updated.emit(); });

    editor->show();
}
} // namespace horizon
