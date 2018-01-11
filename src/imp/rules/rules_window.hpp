#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common/common.hpp"
#include "util/uuid.hpp"
#include "rules/rules.hpp"
namespace horizon {

	class RulesWindow: public Gtk::Window{
		public:
			RulesWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& x, class CanvasGL *ca, class Rules *ru, class Core *c);
			static RulesWindow* create(Gtk::Window *p, class CanvasGL *ca, class Rules *ru, class Core *c);
			typedef sigc::signal<void, Coordi, UUID> type_signal_goto;
			type_signal_goto signal_goto() {return s_signal_goto;}

			typedef sigc::signal<void> type_signal_canvas_update;
			type_signal_canvas_update signal_canvas_update() {return s_signal_canvas_update;}

		private :
			Gtk::ListBox *lb_rules = nullptr;
			Gtk::ListBox *lb_multi = nullptr;
			Gtk::Revealer *rev_multi = nullptr;
			Gtk::Button *button_rule_instance_add = nullptr;
			Gtk::Button *button_rule_instance_remove = nullptr;
			Gtk::Button *button_rule_instance_move_up = nullptr;
			Gtk::Button *button_rule_instance_move_down = nullptr;
			Gtk::Box *rule_editor_box = nullptr;
			Gtk::Button *run_button = nullptr;
			Gtk::Button *apply_button = nullptr;
			Gtk::Stack *stack = nullptr;
			Glib::RefPtr<Gtk::SizeGroup> sg_order;

			void rule_selected(RuleID id);
			void rule_instance_selected(RuleID id, const UUID &uu);
			void update_rule_instances(RuleID id);
			void update_rule_instance_labels();
			void update_rules_enabled();
			void run_checks();

			CanvasGL *canvas = nullptr;
			Rules *rules = nullptr;
			Core *core = nullptr;
			class CanvasAnnotation *annotation;

			class Block *get_block();
			type_signal_goto s_signal_goto;
			type_signal_canvas_update s_signal_canvas_update;
			RuleID rule_current = RuleID::NONE;

			class RuleEditor *editor = nullptr;
			void show_editor(RuleEditor *e);
			RuleEditor *create_editor(Rule *r);

			class TreeColumns : public Gtk::TreeModelColumnRecord {
				public:
					TreeColumns() {
						Gtk::TreeModelColumnRecord::add( name ) ;
						Gtk::TreeModelColumnRecord::add( result ) ;
						Gtk::TreeModelColumnRecord::add( comment ) ;
						Gtk::TreeModelColumnRecord::add( has_location ) ;
						Gtk::TreeModelColumnRecord::add( location ) ;
						Gtk::TreeModelColumnRecord::add( sheet ) ;
					}
					Gtk::TreeModelColumn<Glib::ustring> name;
					Gtk::TreeModelColumn<RulesCheckErrorLevel> result;
					Gtk::TreeModelColumn<Glib::ustring> comment;
					Gtk::TreeModelColumn<bool> has_location;
					Gtk::TreeModelColumn<Coordi> location;
					Gtk::TreeModelColumn<UUID> sheet;
			} ;
			TreeColumns tree_columns;

			Glib::RefPtr<Gtk::TreeStore> check_result_store;
			Gtk::TreeView *check_result_treeview = nullptr;
	};
}
