#include "schematic_properties.hpp"
#include "schematic/schematic.hpp"
#include <iostream>
#include <deque>
#include <algorithm>

namespace horizon {

	class SheetEditor: public Gtk::Grid {
		public:
			SheetEditor(Sheet *s, Schematic *c);

		private:
			Sheet *sheet;
			Schematic *sch;
			int top = 0;
			void append_widget(const std::string &label, Gtk::Widget *w);

	};

	void SheetEditor::append_widget(const std::string &label, Gtk::Widget *w) {
		auto la = Gtk::manage(new Gtk::Label(label));
		la->get_style_context()->add_class("dim-label");
		la->show();
		attach(*la, 0, top, 1, 1);

		w->show();
		w->set_hexpand(true);
		attach(*w, 1, top, 1, 1);
		top++;
	}

	SheetEditor::SheetEditor(Sheet *s, Schematic *c): Gtk::Grid(), sheet(s), sch(c) {
		set_column_spacing(10);
		set_row_spacing(10);
		set_margin_bottom(20);
		set_margin_top(20);
		set_margin_end(20);
		set_margin_start(20);

		auto title_entry = Gtk::manage(new Gtk::Entry());
		title_entry->set_text(s->name);
		title_entry->signal_changed().connect([this, title_entry] {
			auto ti = title_entry->get_text();
			sheet->name = ti;
			auto stack = dynamic_cast<Gtk::Stack*>(get_ancestor(GTK_TYPE_STACK));
			stack->child_property_title(*this) = "Sheet "+ti;
		});

		append_widget("Title", title_entry);



		auto format_combo = Gtk::manage(new Gtk::ComboBoxText());
		format_combo->append(std::to_string(static_cast<int>(Frame::Format::A3_LANDSCAPE)), "A3 Landscape");
		format_combo->append(std::to_string(static_cast<int>(Frame::Format::A4_LANDSCAPE)), "A4 Landscape");
		format_combo->set_active_id(std::to_string(static_cast<int>(sheet->frame.format)));
		format_combo->signal_changed().connect([this, format_combo] {
			sheet->frame.format = static_cast<Frame::Format>(std::stoi(format_combo->get_active_id()));
		});
		append_widget("Format", format_combo);




	}

	SchematicPropertiesDialog::SchematicPropertiesDialog(Gtk::Window *parent, Schematic *c) :
		Gtk::Dialog("Schematic properties", *parent, Gtk::DialogFlags::DIALOG_MODAL|Gtk::DialogFlags::DIALOG_USE_HEADER_BAR),
		sch(c)
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

		for(auto &it: sch->sheets) {
			auto ed = Gtk::manage(new SheetEditor(&it.second, sch));
			stack->add(*ed, (std::string)it.second.uuid, "Sheet "+it.second.name);
		}

		get_content_area()->pack_start(*box, true, true, 0);
		get_content_area()->set_border_width(0);


		show_all();
	}
}
