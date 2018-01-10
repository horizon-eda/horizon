#include "rule_editor_single_pin_net.hpp"
#include "schematic/rule_single_pin_net.hpp"

namespace horizon {
	void RuleEditorSinglePinNet::populate() {
		rule2 = dynamic_cast<RuleSinglePinNet*>(rule);

		Gtk::CheckButton *editor = Gtk::manage(new Gtk::CheckButton("Include unnamed nets"));
		editor->set_valign(Gtk::ALIGN_START);
		pack_start(*editor, true, true, 0);
		editor->set_active(rule2->include_unnamed);
		editor->signal_toggled().connect([this, editor]{
			rule2->include_unnamed = editor->get_active();
			s_signal_updated.emit();
		});
		editor->show();
	}
}
