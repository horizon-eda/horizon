#include "rule_editor_clearance_silk_exp_copper.hpp"
#include "board/rule_clearance_silk_exp_copper.hpp"
#include "widgets/spin_button_dim.hpp"

namespace horizon {
	void RuleEditorClearanceSilkscreenExposedCopper::populate() {
		rule2 = dynamic_cast<RuleClearanceSilkscreenExposedCopper*>(rule);

		builder = Gtk::Builder::create_from_resource("/net/carrotIndustries/horizon/imp/rules/rule_editor_clearance_silk_exp_copper.ui");
		Gtk::Box *editor;
		builder->get_widget("editor", editor);
		pack_start(*editor, true, true, 0);
		auto sp_top = create_spinbutton("top_cl_box");
		auto sp_bot = create_spinbutton("bottom_cl_box");
		sp_top->set_range(0, 100_mm);
		sp_bot->set_range(0, 100_mm);
		sp_top->set_value(rule2->clearance_top);
		sp_bot->set_value(rule2->clearance_bottom);
		sp_top->signal_value_changed().connect([this, sp_top]{
			rule2->clearance_top = sp_top->get_value_as_int();
			s_signal_updated.emit();
		});
		sp_bot->signal_value_changed().connect([this, sp_bot]{
			rule2->clearance_bottom = sp_bot->get_value_as_int();
			s_signal_updated.emit();
		});
		editor->show();
	}
}
