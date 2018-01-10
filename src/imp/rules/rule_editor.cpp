#include "rule_editor.hpp"
#include "widgets/spin_button_dim.hpp"
#include "rule_match_editor.hpp"
#include "rules/rule_match.hpp"

namespace horizon {
	RuleEditor::RuleEditor(Rule *r, class Core *c): Gtk::Box(Gtk::ORIENTATION_VERTICAL, 20), rule(r), core(c) {
		enable_cb = Gtk::manage(new Gtk::CheckButton("Enable this rule"));
		enable_cb->set_margin_start(20);
		enable_cb->set_margin_top(20);
		pack_start(*enable_cb, false, false,0);
		enable_cb->show();
		enable_cb->set_active(rule->enabled);
		enable_cb->signal_toggled().connect([this]{
			rule->enabled = enable_cb->get_active();
			s_signal_updated.emit();
		});
	}

	void RuleEditor::populate() {
		auto la = Gtk::manage(new Gtk::Label(static_cast<std::string>(rule->uuid)));
		pack_start(*la, true, true, 0);
		la->show();
	}

	SpinButtonDim *RuleEditor::create_spinbutton(const char *into) {
		Gtk::Box *box;
		builder->get_widget(into, box);
		SpinButtonDim *r = Gtk::manage(new SpinButtonDim());
		r->show();
		box->pack_start(*r, true, true, 0);
		return r;
	}

	RuleMatchEditor *RuleEditor::create_rule_match_editor(const char *into, RuleMatch *match) {
		Gtk::Box *box;
		builder->get_widget(into, box);
		RuleMatchEditor *r = Gtk::manage(new RuleMatchEditor(match, core));
		r->show();
		box->pack_start(*r, true, true, 0);
		return r;
	}
}
