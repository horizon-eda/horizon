#include "rule_editor_package_checks.hpp"
#include "package/rule_package_checks.hpp"

namespace horizon {

	static const char *text = "This rule checks if the package contains all required objects:\n"
			"• Refdes text on silkscreen\n"
			"• Refdes text on package\n"
			"• Silkscreen refdes width and size\n"
			"• Courtyards polygon\n"
			"• Pad naming convention\n"
			"• No polygons on silkscreen\n";

	void RuleEditorPackageChecks::populate() {
		rule2 = dynamic_cast<RulePackageChecks*>(rule);

		Gtk::Label *editor = Gtk::manage(new Gtk::Label);
		editor->set_text(text);
		editor->set_xalign(0);
		editor->set_valign(Gtk::ALIGN_START);
		editor->set_margin_start(20);
		pack_start(*editor, true, true, 0);
		editor->show();
	}
}
