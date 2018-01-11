#include "rule_editor_clearance_copper_other.hpp"
#include "board/rule_clearance_copper_other.hpp"
#include "widgets/spin_button_dim.hpp"
#include "rule_match_editor.hpp"
#include "core/core.hpp"
#include "widgets/chooser_buttons.hpp"
#include "widgets/parameter_set_editor.hpp"
#include "core/core_board.hpp"
#include "common/patch_type_names.hpp"

namespace horizon {

	static const std::vector<PatchType> patch_types_cu = {
		PatchType::TRACK, PatchType::PAD, PatchType::PAD_TH, PatchType::PLANE, PatchType::VIA
	};

	static const std::vector<PatchType> patch_types_ncu = {
		PatchType::HOLE_NPTH, PatchType::BOARD_EDGE, PatchType::OTHER
	};

	void RuleEditorClearanceCopperOther::populate() {
		rule2 = dynamic_cast<RuleClearanceCopperOther*>(rule);

		auto editor = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 20));
		editor->set_margin_start(20);
		editor->set_margin_end(20);
		editor->set_margin_bottom(20);

		auto grid = Gtk::manage(new Gtk::Grid());
		grid->set_row_spacing(10);
		grid->set_column_spacing(20);

		{
			auto la = Gtk::manage(new Gtk::Label("Match"));
			la->get_style_context()->add_class("dim-label");
			la->set_halign(Gtk::ALIGN_START);
			grid->attach(*la, 0,0, 1,1);
		}
		{
			auto la = Gtk::manage(new Gtk::Label("Layer"));
			la->get_style_context()->add_class("dim-label");
			la->set_halign(Gtk::ALIGN_START);
			grid->attach(*la, 1,0, 1,1);
		}
		{
			auto la = Gtk::manage(new Gtk::Label("Routing offset"));
			la->get_style_context()->add_class("dim-label");
			la->set_halign(Gtk::ALIGN_START);
			grid->attach(*la, 2,0, 1,1);
		}

		auto match_editor = Gtk::manage(new RuleMatchEditor(&rule2->match, core));
		match_editor->set_orientation(Gtk::ORIENTATION_HORIZONTAL);
		match_editor->signal_updated().connect([this] {s_signal_updated.emit();});
		match_editor->set_hexpand(true);
		grid->attach(*match_editor, 0, 1, 1, 1);

		auto *layer_combo = Gtk::manage(new Gtk::ComboBoxText());
		for(const auto &it: core->get_layer_provider()->get_layers()) {
			if(it.second.copper)
				layer_combo->insert(0, std::to_string(it.first), it.second.name + ": " + std::to_string(it.first));
		}
		layer_combo->insert(0, "10000", "Any layer");
		layer_combo->set_hexpand(true);
		layer_combo->set_active_id(std::to_string(rule2->layer));
		layer_combo->signal_changed().connect([this, layer_combo] {
			rule2->layer = std::stoi(layer_combo->get_active_id());
			s_signal_updated.emit();
		});
		grid->attach(*layer_combo, 1, 1, 1, 1);

		auto sp_routing_offset = Gtk::manage(new SpinButtonDim);
		sp_routing_offset->set_range(0, 10_mm);
		sp_routing_offset->set_value(rule2->routing_offset);
		sp_routing_offset->signal_value_changed().connect([this, sp_routing_offset]{
			rule2->routing_offset = sp_routing_offset->get_value_as_int();
			s_signal_updated.emit();
		});
		grid->attach(*sp_routing_offset, 2, 1, 1, 1);




		grid->show_all();
		editor->pack_start(*grid, false, false, 0);

		auto clearance_grid = Gtk::manage(new Gtk::Grid);
		clearance_grid->set_row_spacing(10);
		clearance_grid->set_column_spacing(10);

		{
			int left = 1;
			for(const auto it: patch_types_cu) {
				auto *box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 5));

				auto *bu = Gtk::manage(new Gtk::Button());
				bu->set_image_from_icon_name("pan-down-symbolic", Gtk::ICON_SIZE_BUTTON);
				bu->get_style_context()->add_class("imp-rule-editor-tiny-button-column");
				bu->set_tooltip_text("Set column");
				bu->signal_clicked().connect([this, left]{set_some(-1, left);});
				box->pack_start(*bu, false, false, 0);

				auto *la = Gtk::manage(new Gtk::Label(patch_type_names.at(it)));
				la->set_xalign(0);
				box->pack_start(*la, true, true, 0);



				clearance_grid->attach(*box, left, 0, 1, 1);
				left++;
			}
			int top = 1;
			for(const auto it: patch_types_ncu) {
				auto *box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 5));
				auto *la = Gtk::manage(new Gtk::Label(patch_type_names.at(it)));
				la->set_xalign(1);
				box->pack_start(*la, true, true, 0);

				auto *bu = Gtk::manage(new Gtk::Button());
				bu->set_image_from_icon_name("pan-end-symbolic", Gtk::ICON_SIZE_BUTTON);
				bu->get_style_context()->add_class("imp-rule-editor-tiny-button-row");
				bu->set_tooltip_text("Set row");
				bu->signal_clicked().connect([this, top]{set_some(top, -1);});
				box->pack_start(*bu, false, false, 0);

				clearance_grid->attach(*box, 0, top, 1, 1);
				top++;
			}
		}
		{
			auto *bu = Gtk::manage(new Gtk::Button("set all"));
			bu->get_style_context()->add_class("imp-rule-editor-tiny-button-column");
			bu->signal_clicked().connect([this]{set_some(-1, -1);});
			clearance_grid->attach(*bu, 0, 0, 1, 1);
		}
		{
			int left = 1;
			for(const auto it_col: patch_types_cu) {
				int top = 1;
				for(const auto it_row: patch_types_ncu) {
					auto *sp = Gtk::manage(new SpinButtonDim());
					sp->set_range(0, 100_mm);
					sp->set_value(rule2->get_clearance(it_col, it_row));
					sp->signal_value_changed().connect([this, sp, it_row, it_col] {
						rule2->set_clearance(it_col, it_row, sp->get_value_as_int());
					});
					clearance_grid->attach(*sp, left, top, 1, 1);
					spin_buttons.emplace(std::piecewise_construct, std::forward_as_tuple(top, left), std::forward_as_tuple(sp));
					top++;
				}
				left++;
			}
		}

		clearance_grid->show_all();
		editor->pack_start(*clearance_grid, false, false, 0);


		pack_start(*editor, true, true, 0);
		editor->show();
	}

	void RuleEditorClearanceCopperOther::set_some(int row, int col) {
		Dialogs dias;
		dias.set_parent(dynamic_cast<Gtk::Window*>(get_ancestor(GTK_TYPE_WINDOW)));
		auto res = dias.ask_datum("Enter clearance", 0.1_mm);
		if(!res.first)
			return;
		for(auto &it: spin_buttons) {
			if((it.first.first == row || row == -1) && (it.first.second == col || col == -1)) {
				it.second->set_value(res.second);
			}
		}
	}
}
