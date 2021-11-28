#include "rule_editor_shorted_pads.hpp"
#include "board/rule_shorted_pads.hpp"
#include "rules/rule_match_component.hpp"
#include "rule_match_component_editor.hpp"
#include "rule_match_editor.hpp"

namespace horizon {
void RuleEditorShortedPads::populate()
{
    rule2 = &dynamic_cast<RuleShortedPads &>(rule);

    auto editor = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 20));
    editor->set_margin_start(20);
    editor->set_margin_end(20);
    editor->set_margin_bottom(20);

    auto grid = Gtk::manage(new Gtk::Grid());
    grid->set_row_spacing(10);
    grid->set_column_spacing(20);

    {
        auto la = Gtk::manage(new Gtk::Label("Match Component"));
        la->get_style_context()->add_class("dim-label");
        la->set_halign(Gtk::ALIGN_START);
        grid->attach(*la, 0, 0, 1, 1);
    }
    {
        auto la = Gtk::manage(new Gtk::Label("Match Net"));
        la->get_style_context()->add_class("dim-label");
        la->set_halign(Gtk::ALIGN_START);
        grid->attach(*la, 1, 0, 1, 1);
    }

    auto match_component_editor = Gtk::manage(new RuleMatchComponentEditor(rule2->match_component, core));
    match_component_editor->set_orientation(Gtk::ORIENTATION_HORIZONTAL);
    match_component_editor->signal_updated().connect([this] { s_signal_updated.emit(); });
    grid->attach(*match_component_editor, 0, 1, 1, 1);

    auto match_editor = Gtk::manage(new RuleMatchEditor(rule2->match, core));
    match_editor->set_orientation(Gtk::ORIENTATION_HORIZONTAL);
    match_editor->signal_updated().connect([this] { s_signal_updated.emit(); });
    grid->attach(*match_editor, 1, 1, 1, 1);

    grid->show_all();
    editor->pack_start(*grid, false, false, 0);

    pack_start(*editor, true, true, 0);
    editor->show();
}
} // namespace horizon
