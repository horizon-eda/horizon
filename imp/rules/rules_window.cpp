#include "canvas/canvas.hpp"
#include "util.hpp"
#include "rules_window.hpp"
#include "rules/rules.hpp"
#include "rules/rule_descr.hpp"
#include "rule_editor.hpp"
#include "rule_editor_hole_size.hpp"
#include "rule_editor_clearance_silk_exp_copper.hpp"
#include "rule_editor_track_width.hpp"
#include "rule_editor_clearance_copper.hpp"
#include "rule_editor_single_pin_net.hpp"
#include "rule_editor_via.hpp"
#include "rule_editor_clearance_npth_copper.hpp"
#include "widgets/cell_renderer_layer_display.hpp"
#include "rules/rules_with_core.hpp"
#include "rules/cache.hpp"

#include "core/core_board.hpp"
#include "core/core_schematic.hpp"

namespace horizon {

	class RuleLabel: public Gtk::Box {
		public:
			RuleLabel(Glib::RefPtr<Gtk::SizeGroup> sg, const std::string &t, RuleID i, const UUID &uu = UUID(), int o=-1): Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4), id(i), uuid(uu), order(o) {
				set_margin_top(5);
				set_margin_bottom(5);
				set_margin_left(5);
				set_margin_right(5);

				label_order = Gtk::manage(new Gtk::Label(std::to_string(o)));
				label_order->set_xalign(1);
				label_text = Gtk::manage(new Gtk::Label());
				label_text->set_markup(t);
				label_text->set_xalign(0);
				pack_start(*label_order, false, false, 0);
				pack_start(*label_text, true, true, 0);
				if(o >= 0) {
					label_order->show();
					sg->add_widget(*label_order);
				}
				label_text->show();
			}

			void set_text(const std::string &t) {
				label_text->set_markup(t);
			}

			void set_order(int o ) {
				order = o;
				label_order->set_text(std::to_string(order));
			}
			int get_order() const {
				return order;
			}
			const RuleID id;
			const UUID uuid;


		private:
			int order=0;
			Gtk::Label *label_order = nullptr;
			Gtk::Label *label_text = nullptr;

	};

	RulesWindow::RulesWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& x, CanvasGL *ca, Rules *ru, class Core *c) :
		Gtk::Window(cobject), canvas(ca), rules(ru), core(c) {
		x->get_widget("lb_rules", lb_rules);
		x->get_widget("lb_multi", lb_multi);
		x->get_widget("rev_multi", rev_multi);
		x->get_widget("button_rule_instance_add", button_rule_instance_add);
		x->get_widget("button_rule_instance_remove", button_rule_instance_remove);
		x->get_widget("button_rule_instance_move_up", button_rule_instance_move_up);
		x->get_widget("button_rule_instance_move_down", button_rule_instance_move_down);
		x->get_widget("rule_editor_box", rule_editor_box);
		x->get_widget("results_tv", check_result_treeview);
		x->get_widget("run_button", run_button);
		x->get_widget("apply_button", apply_button);
		x->get_widget("stack", stack);
		sg_order = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);

		lb_rules->signal_row_selected().connect([this](Gtk::ListBoxRow *row){rule_selected(dynamic_cast<RuleLabel*>(row->get_child())->id);});
		for(const auto id: rules->get_rule_ids()) {
			const auto &desc = rule_descriptions.at(id);
			auto la = Gtk::manage(new RuleLabel(sg_order, desc.name, id));
			la->show();
			lb_rules->append(*la);
		}
		lb_multi->signal_selected_rows_changed().connect([this]{
			bool selected = lb_multi->get_selected_row()!=nullptr;
			button_rule_instance_remove->set_sensitive(selected);
			button_rule_instance_move_up->set_sensitive(selected);
			button_rule_instance_move_down->set_sensitive(selected);
		});
		lb_multi->signal_row_selected().connect([this](Gtk::ListBoxRow *row){
			if(row) {
				auto la = dynamic_cast<RuleLabel*>(row->get_child());
				rule_instance_selected(la->id, la->uuid);
			}
			else {
				show_editor(nullptr);
			}
		});
		lb_multi->set_sort_func([this](Gtk::ListBoxRow *a, Gtk::ListBoxRow *b) {
			auto la = dynamic_cast<RuleLabel*>(a->get_child());
			auto lb = dynamic_cast<RuleLabel*>(b->get_child());
			return la->get_order()-lb->get_order();
		});

		button_rule_instance_remove->signal_clicked().connect([this] {
			auto row = lb_multi->get_selected_row();
			if(row) {
				auto la = dynamic_cast<RuleLabel*>(row->get_child());
				rules->remove_rule(la->id, la->uuid);
				update_rule_instances(la->id);
			}
		});

		button_rule_instance_add->signal_clicked().connect([this]{
			rules->add_rule(rule_current);
			update_rule_instances(rule_current);
			lb_multi->select_row(*lb_multi->get_row_at_index(0));
		});

		button_rule_instance_move_up->signal_clicked().connect([this]{
			auto row = lb_multi->get_selected_row();
			if(row) {
				auto la = dynamic_cast<RuleLabel*>(row->get_child());
				rules->move_rule(la->id, la->uuid, -1);
				update_rule_instance_labels();
			}
		});
		button_rule_instance_move_down->signal_clicked().connect([this]{
			auto row = lb_multi->get_selected_row();
			if(row) {
				auto la = dynamic_cast<RuleLabel*>(row->get_child());
				rules->move_rule(la->id, la->uuid, 1);
				update_rule_instance_labels();
			}
		});
		run_button->signal_clicked().connect(sigc::mem_fun(this, &RulesWindow::run_checks));
		apply_button->signal_clicked().connect([this] {
			for(auto &rule: rules->get_rule_ids()) {
				rules_apply(rules, rule, core);
			}
			core->rebuild();
			s_signal_canvas_update.emit();
		});
		update_rules_enabled();

		check_result_store = Gtk::TreeStore::create(tree_columns);
		check_result_treeview->set_model(check_result_store);

		check_result_treeview->append_column("Rule", tree_columns.name);
		{
			auto cr = Gtk::manage(new CellRendererLayerDisplay());
			auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Result", *cr));
			auto cr2 = Gtk::manage(new Gtk::CellRendererText());
			cr2->property_text()="hallo";

			tvc->set_cell_data_func(*cr2, [this](Gtk::CellRenderer* tcr, const Gtk::TreeModel::iterator &it){
				Gtk::TreeModel::Row row = *it;
				auto mcr = dynamic_cast<Gtk::CellRendererText*>(tcr);
				mcr->property_text() = rules_check_error_level_to_string(row[tree_columns.result]);
			});
			tvc->set_cell_data_func(*cr, [this](Gtk::CellRenderer* tcr, const Gtk::TreeModel::iterator &it){
				Gtk::TreeModel::Row row = *it;
				auto mcr = dynamic_cast<CellRendererLayerDisplay*>(tcr);
				auto co = rules_check_error_level_to_color(row[tree_columns.result]);
				Gdk::RGBA va;
				va.set_red(co.r);
				va.set_green(co.g);
				va.set_blue(co.b);
				mcr->property_color() = va;
			});
			tvc->pack_start(*cr2, false);
			check_result_treeview->append_column(*tvc);
		}
		check_result_treeview->append_column("Comment", tree_columns.comment);

		annotation = canvas->create_annotation();

		signal_show().connect([this]{canvas->markers.set_domain_visible(MarkerDomain::CHECK, true); annotation->set_visible(true);});
		signal_hide().connect([this]{canvas->markers.set_domain_visible(MarkerDomain::CHECK, false); annotation->set_visible(false);});

		check_result_treeview->signal_row_activated().connect([this] (const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column) {
			auto it = check_result_store->get_iter(path);
			if(it) {
				Gtk::TreeModel::Row row = *it;
				if(row[tree_columns.has_location]) {
					s_signal_goto.emit(row[tree_columns.location], row[tree_columns.sheet]);
				}

			}
		});

	}

	void RulesWindow::run_checks() {
		check_result_store->freeze_notify();
		check_result_store->clear();
		auto &dom = canvas->markers.get_domain(MarkerDomain::CHECK);
		dom.clear();
		RulesCheckCache cache(core);
		annotation->clear();
		for(auto rule_id: rules->get_rule_ids()) {
			auto result = rules_check(rules, rule_id, core, cache);
			if(result.level != RulesCheckErrorLevel::NOT_RUN || true) {
				Gtk::TreeModel::iterator iter = check_result_store->append();
				Gtk::TreeModel::Row row = *iter;
				row[tree_columns.name] = rule_descriptions.at(rule_id).name;
				row[tree_columns.result] = result.level;
				row[tree_columns.has_location] = false;
				for(const auto &it_err: result.errors) {
					iter = check_result_store->append(row.children());
					Gtk::TreeModel::Row row_err = *iter;
					row_err[tree_columns.result] = it_err.level;
					row_err[tree_columns.comment] = it_err.comment;
					row_err[tree_columns.has_location] = it_err.has_location;
					row_err[tree_columns.location] = it_err.location;
					row_err[tree_columns.sheet] = it_err.sheet;

					if(it_err.has_location) {
						dom.emplace_back(it_err.location, rules_check_error_level_to_color(it_err.level), it_err.sheet);
					}



					for(const auto &path: it_err.error_polygons) {
						ClipperLib::IntPoint last = path.back();
						for(const auto &pt: path) {
							annotation->draw_line({last.X, last.Y}, {pt.X, pt.Y}, ColorP::FROM_LAYER, .01_mm);
							last = pt;
						}
					}
				}
			}
		}
		check_result_store->thaw_notify();
		canvas->update_markers();
		stack->set_visible_child("checks");
	}

	void RulesWindow::rule_selected(RuleID id) {
		auto multi = rule_descriptions.at(id).multi;
		rev_multi->set_reveal_child(multi);
		rule_current = id;
		if(multi) {
			show_editor(nullptr);
			update_rule_instances(id);
		}
		else {
			auto rule = rules->get_rule(id);
			show_editor(create_editor(rule));
		}
	}

	void RulesWindow::rule_instance_selected(RuleID id, const UUID &uu) {
		auto rule = rules->get_rule(id, uu);
		if(rule == nullptr)
			return;
		show_editor(create_editor(rule));
	}

	void RulesWindow::show_editor(RuleEditor *e) {
		if(editor) {
			rule_editor_box->remove(*editor);
			editor = nullptr;
		}
		if(e == nullptr)
			return;
		editor = Gtk::manage(e);
		rule_editor_box->pack_start(*editor, true, true, 0);
		editor->show();
		editor->signal_updated().connect([this] {
			update_rule_instance_labels();
			update_rules_enabled();
		});
	}

	RuleEditor *RulesWindow::create_editor(Rule *r) {
		RuleEditor *e = nullptr;
		switch(r->id) {
			case RuleID::HOLE_SIZE :
				e = new RuleEditorHoleSize(r, core);
			break;

			case RuleID::CLEARANCE_SILKSCREEN_EXPOSED_COPPER :
				e = new RuleEditorClearanceSilkscreenExposedCopper(r, core);
			break;

			case RuleID::PARAMETERS :
				e = new RuleEditorClearanceSilkscreenExposedCopper(r, core);
			break;

			case RuleID::TRACK_WIDTH :
				e = new RuleEditorTrackWidth(r, core);
			break;

			case RuleID::CLEARANCE_COPPER :
				e = new RuleEditorClearanceCopper(r, core);
			break;

			case RuleID::SINGLE_PIN_NET :
				e = new RuleEditorSinglePinNet(r, core);
			break;

			case RuleID::VIA :
				e = new RuleEditorVia(r, core);
			break;

			case RuleID::CLEARANCE_NPTH_COPPER :
				e = new RuleEditorClearanceNPTHCopper(r, core);
			break;

			default:
				e = new RuleEditor(r, core);
		}
		e->populate();
		return e;
	}

	Block *RulesWindow::get_block() {
		if(auto c = dynamic_cast<CoreBoard*>(core)) {
			return c->get_board()->block;
		}
		if(auto c = dynamic_cast<CoreSchematic*>(core)) {
			return c->get_schematic()->block;
		}
		return nullptr;
	}

	void RulesWindow::update_rule_instances(RuleID id) {
		auto inst = rules->get_rules(id);
		auto row = lb_multi->get_selected_row();
		UUID uu;
		if(row) {
			uu = dynamic_cast<RuleLabel*>(row->get_child())->uuid;
		}
		for(auto it: lb_multi->get_children()) {
			delete it;
		}
		for(const auto &it: inst) {
			auto la = Gtk::manage(new RuleLabel(sg_order, it.second->get_brief(get_block()), id, it.first, it.second->order));
			la->set_sensitive(it.second->enabled);
			la->show();
			lb_multi->append(*la);
			if(it.first == uu) {
				auto this_row = dynamic_cast<Gtk::ListBoxRow*>(la->get_parent());
				lb_multi->select_row(*this_row);
			}
		}
		auto children = lb_multi->get_children();
		if(lb_multi->get_selected_row() == nullptr && children.size()>0) {
			lb_multi->select_row(*static_cast<Gtk::ListBoxRow*>(children.front()));
		}
	}

	void RulesWindow::update_rule_instance_labels() {
		for(auto ch: lb_multi->get_children()) {
			auto la = dynamic_cast<RuleLabel*>(dynamic_cast<Gtk::ListBoxRow*>(ch)->get_child());
			auto rule = rules->get_rule(la->id, la->uuid);
			la->set_text(rule->get_brief(get_block()));
			la->set_sensitive(rule->enabled);
			la->set_order(rule->order);
		}
		lb_multi->invalidate_sort();
	}

	void RulesWindow::update_rules_enabled() {
		for(auto ch: lb_rules->get_children()) {
			auto la = dynamic_cast<RuleLabel*>(dynamic_cast<Gtk::ListBoxRow*>(ch)->get_child());
			auto is_multi = rule_descriptions.at(la->id).multi;
			if(!is_multi) {
				auto r = rules->get_rule(la->id);
				la->set_sensitive(r->enabled);
			}

		}
	}

	RulesWindow* RulesWindow::create(Gtk::Window *p, CanvasGL *ca, Rules *ru, Core *c) {
		RulesWindow* w;
		Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
		x->add_from_resource("/net/carrotIndustries/horizon/imp/rules/rules_window.ui");
		x->get_widget_derived("window", w, ca, ru, c);
		w->set_transient_for(*p);
		return w;
	}
}
