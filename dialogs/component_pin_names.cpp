#include "component_pin_names.hpp"
#include <iostream>
#include <deque>
#include <algorithm>

namespace horizon {


	class GatePinEditor: public Gtk::ListBox {
		public:
			GatePinEditor(Component *c, const Gate *g):
				Gtk::ListBox(),
				comp(c), gate(g)
			{
				sg = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);
				set_header_func(sigc::mem_fun(this, &GatePinEditor::header_fun));
				set_selection_mode(Gtk::SELECTION_NONE);
				std::deque<const Pin*> pins_sorted;
				for(const auto &it: gate->unit->pins) {
					pins_sorted.push_back(&it.second);
				}
				std::sort(pins_sorted.begin(), pins_sorted.end(), [](const auto  &a, const auto &b) {return a->primary_name < b->primary_name;});
				for(const auto it: pins_sorted) {
					auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL,16));
					auto la = Gtk::manage(new Gtk::Label(it->primary_name));
					la->set_xalign(0);
					sg->add_widget(*la);
					box->pack_start(*la, false, false, 0);

					auto combo = Gtk::manage(new Gtk::ComboBoxText());

					combo->append(std::to_string(-1), it->primary_name);
					unsigned int i = 0;
					for(const auto &it_pin_name: it->names) {
						combo->append(std::to_string(i++), it_pin_name);
					}
					combo->set_active(0);
					auto path = UUIDPath<2>(gate->uuid, it->uuid);
					if(comp->pin_names.count(path)) {
						combo->set_active(1+comp->pin_names.at(path));
					}

					combo->signal_changed().connect(sigc::bind<Gtk::ComboBoxText*, UUIDPath<2>>(
							sigc::mem_fun(this, &GatePinEditor::changed),
							combo, path
					));
					box->pack_start(*combo, true, true, 0);

					box->set_margin_start(16);
					box->set_margin_end(8);
					box->set_margin_top(4);
					box->set_margin_bottom(4);


					insert(*box, -1);
				}

			}
			Component *comp;
			const Gate *gate;

		private :
			Glib::RefPtr<Gtk::SizeGroup> sg;
			void header_fun(Gtk::ListBoxRow *row, Gtk::ListBoxRow *before) {
				if (before && !row->get_header())	{
					auto ret = Gtk::manage(new Gtk::Separator);
					row->set_header(*ret);
				}
			}

			void changed(Gtk::ComboBoxText* combo, UUIDPath<2> path) {
				std::cout << "ch " << (std::string) path << combo->get_active_row_number() << std::endl;
				comp->pin_names[path] = combo->get_active_row_number()-1;
			}
	};



	ComponentPinNamesDialog::ComponentPinNamesDialog(Gtk::Window *parent, Component *c) :
		Gtk::Dialog("Component "+c->refdes+" pin names", *parent, Gtk::DialogFlags::DIALOG_MODAL|Gtk::DialogFlags::DIALOG_USE_HEADER_BAR),
		comp(c)
		{
		add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
		add_button("OK", Gtk::ResponseType::RESPONSE_OK);
		set_default_response(Gtk::ResponseType::RESPONSE_OK);
		set_default_size(400, 300);

		auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL,0));

		auto stack = Gtk::manage(new Gtk::Stack());
		stack->set_transition_type(Gtk::STACK_TRANSITION_TYPE_SLIDE_RIGHT);
		stack->set_transition_duration(100);
		auto sidebar = Gtk::manage(new Gtk::StackSidebar());
		sidebar->set_stack(*stack);

		box->pack_start(*sidebar, false, false, 0);
		box->pack_start(*stack, true, true, 0);

		std::deque<const Gate*> gates_sorted;
		for(const auto &it: comp->entity->gates) {
			gates_sorted.push_back(&it.second);
		}
		std::sort(gates_sorted.begin(), gates_sorted.end(), [](const auto  &a, const auto &b) {return a->name < b->name;});


		for(auto it: gates_sorted) {
			auto ed = Gtk::manage(new GatePinEditor(comp, it));
			auto sc = Gtk::manage(new Gtk::ScrolledWindow());
			sc->add(*ed);
			stack->add(*sc, (std::string)it->uuid, "Gate "+it->name);
		}


		get_content_area()->pack_start(*box, true, true, 0);

		show_all();
	}
}
