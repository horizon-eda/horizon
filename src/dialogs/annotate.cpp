#include "annotate.hpp"
#include "widgets/net_button.hpp"
#include <iostream>
#include <deque>
#include <algorithm>

namespace horizon {


	class GangedSwitch: public Gtk::Box {
		public:
			GangedSwitch();
			void append(const std::string &id, const std::string &label);
	};

	GangedSwitch::GangedSwitch(): Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 0) {
		get_style_context()->add_class("linked");
	}

	void GangedSwitch::append(const std::string &id, const std::string &label) {
		auto rb = Gtk::manage(new Gtk::RadioButton(label));
		rb->set_mode(false);
		auto ch = get_children();
		if(ch.size()>0) {
			rb->join_group(dynamic_cast<Gtk::RadioButton&>(*ch.front()));
		}
		rb->show();
		pack_start(*rb, false, false, 0);

	}

	AnnotateDialog::AnnotateDialog(Gtk::Window *parent, Schematic *s) :
		Gtk::Dialog("Annotation", *parent, Gtk::DialogFlags::DIALOG_MODAL|Gtk::DialogFlags::DIALOG_USE_HEADER_BAR),
		sch(s)
		{
		add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
		auto ok_button = add_button("Annotate", Gtk::ResponseType::RESPONSE_OK);
		ok_button->signal_clicked().connect(sigc::mem_fun(this, &AnnotateDialog::ok_clicked));
		set_default_response(Gtk::ResponseType::RESPONSE_OK);
		//set_default_size(400, 300);

		auto grid = Gtk::manage(new Gtk::Grid());
		grid->set_column_spacing(10);
		grid->set_row_spacing(10);
		grid->set_margin_bottom(20);
		grid->set_margin_top(20);
		grid->set_margin_end(20);
		grid->set_margin_start(20);
		grid->set_halign(Gtk::ALIGN_CENTER);

		int top = 0;
		{
			auto la = Gtk::manage(new Gtk::Label("Order"));
			la->set_halign(Gtk::ALIGN_END);
			la->get_style_context()->add_class("dim-label");
			w_order = Gtk::manage(new Gtk::ComboBoxText());
			w_order->append(std::to_string(static_cast<int>(Schematic::Annotation::Order::RIGHT_DOWN)), "Right-Down");
			w_order->append(std::to_string(static_cast<int>(Schematic::Annotation::Order::DOWN_RIGHT)), "Down-Right");
			w_order->set_active_id(std::to_string(static_cast<int>(sch->annotation.order)));
			grid->attach(*la, 0, top, 1, 1);
			grid->attach(*w_order, 1, top, 1, 1);
			top++;
		}
		{
			auto la = Gtk::manage(new Gtk::Label("Mode"));
			la->set_halign(Gtk::ALIGN_END);
			la->get_style_context()->add_class("dim-label");
			w_mode = Gtk::manage(new Gtk::ComboBoxText());
			w_mode->append(std::to_string(static_cast<int>(Schematic::Annotation::Mode::SEQUENTIAL)), "Sequential");
			w_mode->append(std::to_string(static_cast<int>(Schematic::Annotation::Mode::SHEET_100)), "Sheet increment 100");
			w_mode->append(std::to_string(static_cast<int>(Schematic::Annotation::Mode::SHEET_1000)), "Sheet increment 1000");
			w_mode->set_active_id(std::to_string(static_cast<int>(sch->annotation.mode)));
			grid->attach(*la, 0, top, 1, 1);
			grid->attach(*w_mode, 1, top, 1, 1);
			top++;
		}
		{
			auto la = Gtk::manage(new Gtk::Label("Keep existing"));
			la->set_halign(Gtk::ALIGN_END);
			la->get_style_context()->add_class("dim-label");
			w_keep = Gtk::manage(new Gtk::Switch());
			w_keep->set_halign(Gtk::ALIGN_START);
			w_keep->set_active(sch->annotation.keep);
			grid->attach(*la, 0, top, 1, 1);
			grid->attach(*w_keep, 1, top, 1, 1);
			top++;
		}
		{
			auto la = Gtk::manage(new Gtk::Label("Fill gaps"));
			la->set_halign(Gtk::ALIGN_END);
			la->get_style_context()->add_class("dim-label");
			w_fill_gaps = Gtk::manage(new Gtk::Switch());
			w_fill_gaps->set_halign(Gtk::ALIGN_START);
			w_fill_gaps->set_active(sch->annotation.fill_gaps);
			grid->attach(*la, 0, top, 1, 1);
			grid->attach(*w_fill_gaps, 1, top, 1, 1);
			top++;
		}



		get_content_area()->pack_start(*grid, true, true, 0);

		show_all();
	}

	void AnnotateDialog::ok_clicked() {
		sch->annotation.order = static_cast<Schematic::Annotation::Order>(std::stoi(w_order->get_active_id()));
		sch->annotation.mode = static_cast<Schematic::Annotation::Mode>(std::stoi(w_mode->get_active_id()));
		sch->annotation.fill_gaps = w_fill_gaps->get_active();
		sch->annotation.keep = w_keep->get_active();
		sch->annotate();
	}
}
