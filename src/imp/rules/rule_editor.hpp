#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common/common.hpp"
#include "util/uuid.hpp"
#include "rules/rule.hpp"
namespace horizon {
	class RuleEditor: public Gtk::Box {
		public:
			RuleEditor(Rule *r, class Core *c);
			virtual void populate();
			typedef sigc::signal<void> type_signal_updated;
			type_signal_updated signal_updated() {return s_signal_updated;}

		private:
			Gtk::CheckButton *enable_cb = nullptr;

		protected:
			Rule *rule;
			class Core *core = nullptr;
			Glib::RefPtr<Gtk::Builder> builder;
			class SpinButtonDim *create_spinbutton(const char *box);
			class RuleMatchEditor *create_rule_match_editor(const char *box, class RuleMatch *match);
			type_signal_updated s_signal_updated;


	};

}
