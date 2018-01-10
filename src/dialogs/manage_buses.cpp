#include "manage_buses.hpp"
#include "widgets/net_button.hpp"
#include <iostream>
#include <deque>
#include <algorithm>

namespace horizon {

	class BusMemberEditor: public Gtk::Box {
		public:
		BusMemberEditor(Bus *b, Bus::Member *m, Block *bl):Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4), bus(b), bus_member(m), block(bl) {
			set_margin_start(8);
			set_margin_end(8);
			set_margin_top(4);
			set_margin_bottom(4);

			entry = Gtk::manage(new Gtk::Entry());
			entry->set_text(bus_member->name);
			entry->set_width_chars(0);
			entry->signal_changed().connect(sigc::mem_fun(this, &BusMemberEditor::member_name_changed));
			pack_start(*entry, true, true, 0);

			auto auto_name_button = Gtk::manage(new Gtk::Button());
			auto_name_button->set_image_from_icon_name("go-next-symbolic", Gtk::ICON_SIZE_BUTTON);
			auto_name_button->signal_clicked().connect([this] {
				bus_member->net->name = bus->name + "_" + bus_member->name;
				net_button->update();
			});
			pack_start(*auto_name_button, false,false, 0);

			net_button = Gtk::manage(new NetButton(block));
			net_button->set_net(bus_member->net->uuid);
			net_button->signal_changed().connect(sigc::mem_fun(this, &BusMemberEditor::bus_net_changed));
			net_button->set_sensitive(!bus_member->net->has_bus_rippers);
			pack_start(*net_button, false,false, 0);


			auto delbutton = Gtk::manage(new Gtk::Button());
			delbutton->set_image_from_icon_name("list-remove-symbolic", Gtk::ICON_SIZE_BUTTON);
			delbutton->set_sensitive(!bus_member->net->has_bus_rippers);
			//delbutton->get_style_context()->add_class("destructive-action");
			delbutton->set_margin_start(16);
			delbutton->signal_clicked().connect(sigc::mem_fun(this, &BusMemberEditor::bus_member_remove));
			pack_start(*delbutton, false, false, 0);
		}
		Gtk::Entry *entry;
		NetButton *net_button;
		private:
			void member_name_changed() {
				std::string new_name = entry->get_text();
				bus_member->name = new_name;
			}

			void bus_net_changed(const UUID &uu) {
				bus_member->net = &block->nets.at(uu);
			}

			void bus_member_remove() {
				bus->members.erase(bus_member->uuid);
				bus_member = nullptr;
				delete this->get_parent();
			}

			Bus *bus;
			Bus::Member *bus_member;
			Block *block;
	};


	class BusEditor: public Gtk::Box {
		public :
			BusEditor(Bus *bu, Block *bl):Gtk::Box(Gtk::ORIENTATION_VERTICAL,0), bus(bu), block(bl) {
				auto labelbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4));
				labelbox->set_margin_start(12);
				labelbox->set_margin_end(4);
				labelbox->set_margin_top(4);
				labelbox->set_margin_bottom(4);
				auto la = Gtk::manage(new Gtk::Label("Bus Name"));
				bus_name_entry = Gtk::manage(new Gtk::Entry());
				bus_name_entry->set_text(bus->name);
				bus_name_entry->signal_changed().connect(sigc::mem_fun(this, &BusEditor::bus_name_changed));
				labelbox->pack_start(*la, false, false, 0);
				labelbox->pack_start(*bus_name_entry, true, true, 0);


				auto add_member_button = Gtk::manage(new Gtk::Button());
				add_member_button->set_margin_end(6);
				add_member_button->set_margin_start(16);
				add_member_button->set_image_from_icon_name("list-add-symbolic", Gtk::ICON_SIZE_BUTTON);
				add_member_button->signal_clicked().connect(sigc::mem_fun(this, &BusEditor::bus_add_member));
				labelbox->pack_start(*add_member_button, false, false, 0);

				pack_start(*labelbox, false, false, 0);

				auto sc = Gtk::manage(new Gtk::ScrolledWindow());
				listbox = Gtk::manage(new Gtk::ListBox());
				listbox->set_header_func(sigc::mem_fun(this, &BusEditor::header_fun));
				listbox->set_selection_mode(Gtk::SELECTION_NONE);
				sc->add(*listbox);
				pack_start(*sc, true, true, 0);

				std::deque<Bus::Member*> members_sorted;
				for(auto &it: bus->members) {
					members_sorted.push_back(&it.second);
				}
				std::sort(members_sorted.begin(), members_sorted.end(), [](const auto  &a, const auto &b) {return a->name < b->name;});

				sg_entry = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);
				sg_button= Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);
				for(auto it: members_sorted) {
					add_row(it);
				}

			}

		private :
			Bus *bus;
			Block *block;
			Gtk::Entry *bus_name_entry;
			Gtk::ListBox *listbox;
			Glib::RefPtr<Gtk::SizeGroup> sg_entry;
			Glib::RefPtr<Gtk::SizeGroup> sg_button;

			void add_row(Bus::Member *mem) {
				auto ed = Gtk::manage(new BusMemberEditor(bus, mem, block));
				sg_entry->add_widget(*ed->entry);
				sg_button->add_widget(*ed->net_button);
				listbox->insert(*ed, -1);
				listbox->show_all();
			}

			void header_fun(Gtk::ListBoxRow *row, Gtk::ListBoxRow *before) {
				if (before && !row->get_header())	{
					auto ret = Gtk::manage(new Gtk::Separator);
					row->set_header(*ret);
				}
			}
			void bus_name_changed() {
				std::string new_name = bus_name_entry->get_text();
				bus->name = new_name;
				auto *stack = dynamic_cast<Gtk::Stack*>(get_parent());
				stack->child_property_title(*this).set_value(new_name);
			}
			void bus_add_member() {
				auto uu = UUID::random();
				Bus::Member *newmember = &bus->members.emplace(uu, uu).first->second;
				newmember->name = "fixme";
				uu = UUID::random();
				Net *newnet = block->insert_net();
				newnet->name = "changeme";
				newnet->is_bussed = true;
				newmember->net = newnet;
				add_row(newmember);

			}
	};




	ManageBusesDialog::ManageBusesDialog(Gtk::Window *parent, Block *bl) :
		Gtk::Dialog("Manage buses", *parent, Gtk::DialogFlags::DIALOG_MODAL|Gtk::DialogFlags::DIALOG_USE_HEADER_BAR),
		block(bl)
		{
		add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
		add_button("OK", Gtk::ResponseType::RESPONSE_OK);
		set_default_response(Gtk::ResponseType::RESPONSE_OK);
		set_default_size(400, 300);

		auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL,0));

		stack = Gtk::manage(new Gtk::Stack());
		stack->set_transition_type(Gtk::STACK_TRANSITION_TYPE_SLIDE_RIGHT);
		stack->set_transition_duration(100);
		stack->property_visible_child_name().signal_changed().connect(sigc::mem_fun(this, &ManageBusesDialog::update_bus_removable));
		auto sidebar = Gtk::manage(new Gtk::StackSidebar());
		sidebar->set_stack(*stack);

		auto box2 = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL,0));
		box2->pack_start(*sidebar, true, true, 0);

		auto cssp = Gtk::CssProvider::create();
		cssp->load_from_data(".bus-toolbar { border-right: 1px solid @borders;}");
		Gtk::StyleContext::add_provider_for_screen(Gdk::Screen::get_default(), cssp, 0);

		auto tb = Gtk::manage(new Gtk::Toolbar());
		tb->set_icon_size(Gtk::ICON_SIZE_MENU);
		tb->set_toolbar_style(Gtk::TOOLBAR_ICONS);
		tb->get_style_context()->add_class("bus-toolbar");
		{
			auto tbo = Gtk::manage(new Gtk::ToolButton());
			tbo->set_icon_name("list-add-symbolic");
			tbo->signal_clicked().connect(sigc::mem_fun(this, &ManageBusesDialog::add_bus));
			//tbo->signal_clicked().connect([this]{s_signal_add_sheet.emit();});
			tb->insert(*tbo, -1);
		}
		{
			auto tbo = Gtk::manage(new Gtk::ToolButton());
			tbo->set_icon_name("list-remove-symbolic");
			tbo->signal_clicked().connect(sigc::mem_fun(this, &ManageBusesDialog::remove_bus));
			tb->insert(*tbo, -1);
			delete_button = tbo;
		}


		box2->pack_start(*tb, false, false, 0);

		box->pack_start(*box2, false, false, 0);
		box->pack_start(*stack, true, true, 0);

		std::deque<Bus*> buses_sorted;
		for(auto &it: block->buses) {
			buses_sorted.push_back(&it.second);
		}
		std::sort(buses_sorted.begin(), buses_sorted.end(), [](const auto  &a, const auto &b) {return a->name < b->name;});


		for(auto it: buses_sorted) {
			auto ed = Gtk::manage(new BusEditor(it, block));
			//auto sc = Gtk::manage(new Gtk::ScrolledWindow());
			//sc->add(*ed);
			stack->add(*ed, (std::string)it->uuid, it->name);
		}


		get_content_area()->pack_start(*box, true, true, 0);
		get_content_area()->set_border_width(0);
		update_bus_removable();
		show_all();
	}

	void ManageBusesDialog::remove_bus() {
		auto bus_current_uuid = UUID(stack->get_visible_child_name());
		const auto bus = block->buses.at(bus_current_uuid);
		if(bus.is_referenced)
			return;
		delete stack->get_visible_child();
		block->buses.erase(bus_current_uuid);
	}

	void ManageBusesDialog::update_bus_removable() {
		auto vc = stack->get_visible_child_name();
		if(vc.size()) {
			auto bus_current_uuid = UUID(stack->get_visible_child_name());
			const auto bus = block->buses.at(bus_current_uuid);
			delete_button->set_sensitive(!bus.is_referenced);
		}
		else {
			delete_button->set_sensitive(false);
		}
	}

	void ManageBusesDialog::add_bus() {
		auto uu = UUID::random();
		Bus *newbus = &block->buses.emplace(uu,uu).first->second;
		newbus->name = "NEW";
		auto ed = Gtk::manage(new BusEditor(newbus, block));
		//auto sc = Gtk::manage(new Gtk::ScrolledWindow());
		//sc->add(*ed);
		stack->add(*ed, (std::string)newbus->uuid, newbus->name);
		stack->show_all();

	}
}
