#include "rule_editor_via.hpp"
#include "board/rule_via.hpp"
#include "widgets/spin_button_dim.hpp"
#include "rule_match_editor.hpp"
#include "core/core.hpp"
#include "widgets/chooser_buttons.hpp"
#include "widgets/parameter_set_editor.hpp"
#include "core/core_board.hpp"

namespace horizon {
	void RuleEditorVia::populate() {
		rule2 = dynamic_cast<RuleVia*>(rule);

		auto editor = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 10));

		auto grid = Gtk::manage(new Gtk::Grid());
		grid->set_row_spacing(10);
		grid->set_column_spacing(20);
		grid->set_margin_start(20);
		grid->set_margin_end(20);
		{
			auto la = Gtk::manage(new Gtk::Label("Match"));
			la->get_style_context()->add_class("dim-label");
			la->set_halign(Gtk::ALIGN_START);
			grid->attach(*la, 0,0, 1,1);
		}
		{
			auto la = Gtk::manage(new Gtk::Label("Padstack"));
			la->get_style_context()->add_class("dim-label");
			la->set_halign(Gtk::ALIGN_START);
			grid->attach(*la, 1,0, 1,1);
		}

		auto match_editor = Gtk::manage(new RuleMatchEditor(&rule2->match, core));
		match_editor->set_orientation(Gtk::ORIENTATION_HORIZONTAL);
		match_editor->signal_updated().connect([this] {s_signal_updated.emit();});
		grid->attach(*match_editor, 0, 1, 1, 1);

		auto c = dynamic_cast<CoreBoard*>(core);
		auto ps_button = Gtk::manage(new ViaPadstackButton(*c->get_via_padstack_provider()));
		ps_button->property_selected_uuid() = rule2->padstack;
		ps_button->property_selected_uuid().signal_changed().connect([this, ps_button] {
			rule2->padstack = ps_button->property_selected_uuid();
		});
		grid->attach(*ps_button, 1, 1, 1, 1);


		grid->show_all();
		editor->pack_start(*grid, false, false, 0);


		auto pse = Gtk::manage(new ParameterSetEditor(&rule2->parameter_set));
		pse->set_button_margin_left(20);
		pse->show();
		editor->pack_start(*pse, true, true, 0);


		pack_start(*editor, true, true, 0);
		editor->show();
	}
}
