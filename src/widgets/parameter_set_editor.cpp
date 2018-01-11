#include "parameter_set_editor.hpp"
#include "spin_button_dim.hpp"
#include "common/common.hpp"

namespace horizon {

	static void header_fun(Gtk::ListBoxRow *row, Gtk::ListBoxRow *before) {
		if (before && !row->get_header())	{
			auto ret = Gtk::manage(new Gtk::Separator);
			row->set_header(*ret);
		}
	}

	Gtk::Widget *ParameterSetEditor::create_extra_widget(ParameterID id) {
		return nullptr;
	}

	class ParameterEditor: public Gtk::Box {
		public:
			ParameterEditor(ParameterID id, ParameterSetEditor *pse): Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8), parent(pse), parameter_id(id) {
				auto la = Gtk::manage(new Gtk::Label(parameter_id_to_name(id)));
				la->get_style_context()->add_class("dim-label");
				la->set_xalign(1);
				parent->sg_label->add_widget(*la);
				pack_start(*la, false, false, 0);

				sp = Gtk::manage(new SpinButtonDim());
				sp->set_range(-1e9, 1e9);
				sp->set_value(parent->parameter_set->at(id));
				sp->signal_value_changed().connect([this] {(*parent->parameter_set)[parameter_id] = sp->get_value_as_int();});
				sp->signal_key_press_event().connect([this] (GdkEventKey* ev){
					if(ev->keyval == GDK_KEY_Return && (ev->state & GDK_SHIFT_MASK)) {
						sp->activate();
						parent->s_signal_apply_all.emit(parameter_id);
						return true;
					}
					return false;
				});
				sp->signal_activate().connect([this] {
					auto widgets = parent->listbox->get_children();
					auto my_row = sp->get_ancestor(GTK_TYPE_LIST_BOX_ROW);
					auto next = std::find(widgets.begin(), widgets.end(), my_row)+1;
					if(next < widgets.end()) {
						auto e = dynamic_cast<ParameterEditor*>(dynamic_cast<Gtk::ListBoxRow*>(*next)->get_child());
						e->sp->grab_focus();
					}
					else {
						parent->s_signal_activate_last.emit();
					}


				});
				pack_start(*sp, true, true, 0);

				auto delete_button = Gtk::manage(new Gtk::Button());
				delete_button->set_image_from_icon_name("list-remove-symbolic", Gtk::ICON_SIZE_BUTTON);
				pack_start(*delete_button, false, false, 0);
				delete_button->signal_clicked().connect([this] {
					parent->parameter_set->erase(parameter_id);
					delete this->get_parent();
				});

				if(auto extra_button = parent->create_extra_widget(id)) {
					pack_start(*extra_button, false, false, 0);
				}


				set_margin_start(4);
				set_margin_end(4);
				set_margin_top(4);
				set_margin_bottom(4);
				show_all();
			}
			SpinButtonDim *sp = nullptr;
		private:
			ParameterSetEditor *parent;
			ParameterID parameter_id;

	};

	ParameterSetEditor::ParameterSetEditor(ParameterSet *ps, bool populate_init): Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0), parameter_set(ps) {
		add_button = Gtk::manage(new Gtk::MenuButton());
		add_button->set_label("Add parameter");
		add_button->set_halign(Gtk::ALIGN_START);
		add_button->set_margin_bottom(10);
		add_button->set_margin_top(10);
		add_button->set_margin_start(10);
		add_button->set_margin_end(10);
		add_button->show();
		pack_start(*add_button, false, false, 0);


		auto popover = Gtk::manage(new Gtk::Popover());
		popover_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 8));
		popover->add(*popover_box);
		add_button->set_popover(*popover);
		popover->signal_show().connect(sigc::mem_fun(this, &ParameterSetEditor::update_popover_box));

		auto sc = Gtk::manage(new Gtk::ScrolledWindow());
		sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
		listbox = Gtk::manage(new Gtk::ListBox());
		listbox->set_selection_mode(Gtk::SELECTION_NONE);
		listbox->set_header_func(&header_fun);
		sc->add(*listbox);

		sg_label = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);
		if(populate_init)
			populate();

		sc->show_all();
		pack_start(*sc, true, true ,0);
	}

	void ParameterSetEditor::set_button_margin_left(int margin) {
		add_button->set_margin_start(margin);
	}

	void ParameterSetEditor::populate() {
		for(auto &it: *parameter_set) {
			auto pe = Gtk::manage(new ParameterEditor(it.first, this));
			listbox->add(*pe);
		}
		listbox->show_all();
	}

	void ParameterSetEditor::focus_first() {
		auto widgets = listbox->get_children();
		if(widgets.size()) {
			auto w = dynamic_cast<ParameterEditor*>(dynamic_cast<Gtk::ListBoxRow*>(widgets.front())->get_child());
			w->sp->grab_focus();
		}
	}

	void ParameterSetEditor::update_popover_box() {
		for(auto it: popover_box->get_children()) {
			delete it;
		}
		for(int i=1; i<(static_cast<int>(ParameterID::N_PARAMETERS)); i++) {
			auto id = static_cast<ParameterID>(i);
			if(parameter_set->count(id) == 0) {
				auto bu = Gtk::manage(new Gtk::Button(parameter_id_to_name(id)));
				bu->signal_clicked().connect([this, id] {
					auto popover = dynamic_cast<Gtk::Popover*>(popover_box->get_ancestor(GTK_TYPE_POPOVER));
					#if GTK_CHECK_VERSION(3,22,0)
						popover->popdown();
					#else
						popover->hide();
					#endif
					(*parameter_set)[id] = 0.5_mm;
					auto pe = Gtk::manage(new ParameterEditor(id, this));
					listbox->add(*pe);
				});
				popover_box->pack_start(*bu, false, false, 0);
			}
		}
		popover_box->show_all();
	}

}
