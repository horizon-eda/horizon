#include "selection_filter_dialog.hpp"

namespace horizon {

	static void header_fun(Gtk::ListBoxRow *row, Gtk::ListBoxRow *before) {
		if (before && !row->get_header())	{
			auto ret = Gtk::manage(new Gtk::Separator);
			row->set_header(*ret);
		}
	}

	SelectionFilterDialog::SelectionFilterDialog(Gtk::Window *parent, SelectionFilter *sf, Core *c) :
		Gtk::Window(),
		selection_filter(sf),
		core(c){
		set_default_size(200, 300);
		set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
		set_transient_for(*parent);
		auto hb = Gtk::manage(new Gtk::HeaderBar());
		hb->set_show_close_button(true);
		set_titlebar(*hb);
		hb->show();
		set_title("Selection filter");

		auto reset_button = Gtk::manage(new Gtk::Button());
		reset_button->set_image_from_icon_name("edit-select-all-symbolic", Gtk::ICON_SIZE_BUTTON);
		reset_button->show_all();
		reset_button->signal_clicked().connect([this]{
			for(auto cb: checkbuttons) {
				cb->set_active(true);
			}
		});
		hb->pack_start(*reset_button);
		reset_button->show();

		/*auto cssp = Gtk::CssProvider::create();
		cssp->load_from_data(".imp-tiny-button {min-width:0px; min-height:0px; padding:0px;}");
		Gtk::StyleContext::add_provider_for_screen(Gdk::Screen::get_default(), cssp, 700);*/

		listbox = Gtk::manage(new Gtk::ListBox());
		listbox->set_selection_mode(Gtk::SELECTION_NONE);
		listbox->set_header_func(sigc::ptr_fun(&header_fun));
		for(const auto &it: object_descriptions) {
			auto ot = it.first;
			if(ot == ObjectType::POLYGON)
				continue;
			if(core->has_object_type(ot)) {
				auto cb = Gtk::manage(new Gtk::CheckButton(it.second.name_pl));
				cb->set_active(true);
				cb->signal_toggled().connect([this, ot, cb] {selection_filter->object_filter[ot]=cb->get_active();});
				checkbuttons.push_back(cb);
				auto bbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 2));


				auto only_button = Gtk::manage(new Gtk::Button());
				only_button->set_margin_start(5);
				//only_button->set_margin_top(2);
				//only_button->set_margin_bottom(1);
				only_button->set_image_from_icon_name("pan-end-symbolic", Gtk::ICON_SIZE_BUTTON);
				only_button->set_relief(Gtk::RELIEF_NONE);
				only_button->signal_clicked().connect([this, ot, cb] {
					for(auto cb_other: checkbuttons) {
						cb_other->set_active(cb_other==cb);
					}
				});

				bbox->pack_start(*only_button, false, false, 0);
				bbox->pack_start(*cb, true, true, 0);

				listbox->append(*bbox);
			}
		}


		auto sc = Gtk::manage(new Gtk::ScrolledWindow());
		sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
		sc->add(*listbox);
		add(*sc);
		sc->show_all();
	}
}
