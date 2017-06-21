#include "manage_via_templates.hpp"
#include "widgets/chooser_buttons.hpp"
#include "widgets/parameter_set_editor.hpp"
#include "board.hpp"
#include "dialogs.hpp"
#include <iostream>
#include <deque>
#include <algorithm>

namespace horizon {




	class ViaTemplateEditor: public Gtk::Box {
		public :
			ViaTemplateEditor(ViaTemplate *vt, ViaPadstackProvider *vpp):Gtk::Box(Gtk::ORIENTATION_VERTICAL,0), via_template(vt) {
				auto labelbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4));
				labelbox->set_margin_start(12);
				labelbox->set_margin_end(4);
				labelbox->set_margin_top(4);
				labelbox->set_margin_bottom(4);
				auto la = Gtk::manage(new Gtk::Label("Name"));
				name_entry = Gtk::manage(new Gtk::Entry());
				name_entry->set_text(vt->name);
				name_entry->signal_changed().connect(sigc::mem_fun(this, &ViaTemplateEditor::name_changed));
				labelbox->pack_start(*la, false, false, 0);
				labelbox->pack_start(*name_entry, true, true, 0);


				auto ps_button = Gtk::manage(new ViaPadstackButton(*vpp));
				ps_button->property_selected_uuid() = vt->padstack->uuid;
				ps_button->set_margin_end(6);
				ps_button->set_margin_start(6);
				labelbox->pack_start(*ps_button, false, false, 0);

				pack_start(*labelbox, false, false, 0);

				auto ed = Gtk::manage(new ParameterSetEditor(&vt->parameter_set));
				pack_start(*ed, true, true, 0);
			}

		private :
			ViaTemplate *via_template;
			Gtk::Entry *name_entry;
			Gtk::ListBox *listbox;
			Glib::RefPtr<Gtk::SizeGroup> sg_entry;
			Glib::RefPtr<Gtk::SizeGroup> sg_button;


			void name_changed() {
				std::string new_name = name_entry->get_text();
				via_template->name = new_name;
				auto *stack = dynamic_cast<Gtk::Stack*>(get_parent());
				stack->child_property_title(*this).set_value(new_name);
			}
	};




	ManageViaTemplatesDialog::ManageViaTemplatesDialog(Gtk::Window *pa, Board *brd, ViaPadstackProvider *vpp) :
		Gtk::Dialog("Manage via templates", *pa, Gtk::DialogFlags::DIALOG_MODAL|Gtk::DialogFlags::DIALOG_USE_HEADER_BAR),
		board(brd), via_padstack_provider(vpp), parent(pa)
		{
		add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
		add_button("OK", Gtk::ResponseType::RESPONSE_OK);
		set_default_response(Gtk::ResponseType::RESPONSE_OK);
		set_default_size(500, 300);

		auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL,0));

		stack = Gtk::manage(new Gtk::Stack());
		stack->set_transition_type(Gtk::STACK_TRANSITION_TYPE_SLIDE_RIGHT);
		stack->set_transition_duration(100);
		stack->property_visible_child_name().signal_changed().connect(sigc::mem_fun(this, &ManageViaTemplatesDialog::update_template_removable));
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
			tbo->signal_clicked().connect(sigc::mem_fun(this, &ManageViaTemplatesDialog::add_template));
			tb->insert(*tbo, -1);
		}
		{
			auto tbo = Gtk::manage(new Gtk::ToolButton());
			tbo->set_icon_name("list-remove-symbolic");
			tbo->signal_clicked().connect(sigc::mem_fun(this, &ManageViaTemplatesDialog::remove_template));
			tb->insert(*tbo, -1);
			delete_button = tbo;
		}


		box2->pack_start(*tb, false, false, 0);

		box->pack_start(*box2, false, false, 0);
		box->pack_start(*stack, true, true, 0);

		std::deque<ViaTemplate*> vts_sorted;
		for(auto &it: board->via_templates) {
			vts_sorted.push_back(&it.second);
		}
		std::sort(vts_sorted.begin(), vts_sorted.end(), [](const auto  &a, const auto &b) {return a->name < b->name;});

		for(auto it: vts_sorted) {
			auto ed = Gtk::manage(new ViaTemplateEditor(it, via_padstack_provider));
			stack->add(*ed, (std::string)it->uuid, it->name);
		}


		get_content_area()->pack_start(*box, true, true, 0);
		get_content_area()->set_border_width(0);
		update_template_removable();
		show_all();
	}

	void ManageViaTemplatesDialog::remove_template() {
		auto current_uuid = UUID(stack->get_visible_child_name());
		const auto &vt = board->via_templates.at(current_uuid);
		if(vt.is_referenced)
			return;
		delete stack->get_visible_child();
		board->via_templates.erase(current_uuid);
	}

	void ManageViaTemplatesDialog::update_template_removable() {
		auto vc = stack->get_visible_child_name();
		if(vc.size()) {
			auto current_uuid = UUID(stack->get_visible_child_name());
			const auto &vt = board->via_templates.at(current_uuid);
			delete_button->set_sensitive(!vt.is_referenced);
		}
		else {
			delete_button->set_sensitive(false);
		}
	}

	void ManageViaTemplatesDialog::add_template() {
		Dialogs dias;
		dias.set_parent(parent);
		auto r = dias.select_via_padstack(via_padstack_provider);
		if(r.first) {
			auto ps = via_padstack_provider->get_padstack(r.second);
			auto uu = UUID::random();
			ViaTemplate *vt= &board->via_templates.emplace(std::piecewise_construct, std::forward_as_tuple(uu), std::forward_as_tuple(uu, ps)).first->second;

			vt->name = "New";

			auto ed = Gtk::manage(new ViaTemplateEditor(vt, via_padstack_provider));
			stack->add(*ed, (std::string)vt->uuid, vt->name);
			stack->show_all();
		}


	}
}
