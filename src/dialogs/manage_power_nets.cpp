#include "manage_power_nets.hpp"
#include <iostream>
#include <deque>
#include <algorithm>
#include "util/gtk_util.hpp"

namespace horizon {

	class PowerNetEditor: public Gtk::Box {
		public:
			PowerNetEditor(Net *n, Block *bl): Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4), net(n), block(bl) {
				auto entry = Gtk::manage(new Gtk::Entry());
				entry->set_text(net->name);
				entry->signal_changed().connect([this, entry] {net->name=entry->get_text();});
				pack_start(*entry, true, true, 0);

				auto combo = Gtk::manage(new Gtk::ComboBoxText);

				static const std::map<Net::PowerSymbolStyle, std::string> lut = {
					{Net::PowerSymbolStyle::GND, "Ground"},
					{Net::PowerSymbolStyle::DOT, "Dot"},
					{Net::PowerSymbolStyle::ANTENNA, "Antenna"},
				};
				bind_widget(combo, lut, net->power_symbol_style);

				pack_start(*combo, false, false, 0);


				delete_button = Gtk::manage(new Gtk::Button());
				delete_button->set_image_from_icon_name("list-remove-symbolic", Gtk::ICON_SIZE_BUTTON);
				delete_button->set_sensitive(net->n_pins_connected == 0 && net->is_power_forced == false);
				pack_start(*delete_button, false, false, 0);
				delete_button->signal_clicked().connect([this] {
					block->nets.erase(net->uuid);
					delete this->get_parent();
				});

				set_margin_start(4);
				set_margin_end(4);
				set_margin_top(4);
				set_margin_bottom(4);
				show_all();
			}
			Gtk::Button *delete_button = nullptr;

		private:
			Net *net;
			Block *block;
	};

	ManagePowerNetsDialog::ManagePowerNetsDialog(Gtk::Window *parent, Block *bl) :
		Gtk::Dialog("Manage power nets", *parent, Gtk::DialogFlags::DIALOG_MODAL|Gtk::DialogFlags::DIALOG_USE_HEADER_BAR),
		block(bl)
		{
		add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
		add_button("OK", Gtk::ResponseType::RESPONSE_OK);
		set_default_response(Gtk::ResponseType::RESPONSE_OK);
		set_default_size(400, 300);



		auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
		auto add_button = Gtk::manage(new Gtk::Button("Add Power net"));
		add_button->signal_clicked().connect(sigc::mem_fun(this, &ManagePowerNetsDialog::handle_add_power_net));
		add_button->set_halign(Gtk::ALIGN_START);
		add_button->set_margin_bottom(8);
		add_button->set_margin_top(8);
		add_button->set_margin_start(8);
		add_button->set_margin_end(8);

		box->pack_start(*add_button, false, false, 0);


		auto sc = Gtk::manage(new Gtk::ScrolledWindow());
		sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
		listbox = Gtk::manage(new Gtk::ListBox());
		listbox->set_selection_mode(Gtk::SELECTION_NONE);
		listbox->set_header_func(sigc::ptr_fun(header_func_separator));
		sc->add(*listbox);
		box->pack_start(*sc, true, true ,0);


		for(auto &it: block->nets) {
			if(it.second.is_power) {
				auto ne = Gtk::manage(new PowerNetEditor(&it.second, block));
				listbox->add(*ne);
			}
		}

		get_content_area()->pack_start(*box, true, true, 0);
		get_content_area()->set_border_width(0);
		show_all();
	}

	void ManagePowerNetsDialog::handle_add_power_net() {
		auto net = block->insert_net();
		net->name = "GND";
		net->is_power = true;

		auto ne = Gtk::manage(new PowerNetEditor(net, block));
		listbox->add(*ne);
	}
}
