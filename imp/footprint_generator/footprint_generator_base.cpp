#include "footprint_generator_base.hpp"
namespace horizon {
	FootprintGeneratorBase::FootprintGeneratorBase(const char *resource, CorePackage *c) :Glib::ObjectBase (typeid(FootprintGeneratorBase)), Gtk::Box(Gtk::ORIENTATION_VERTICAL, 4),  p_property_can_generate(*this, "can-generate"), core(c)
	{
		box_top = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
		pack_start(*box_top, false, false, 0);
		{
			auto tbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4));
			auto la = Gtk::manage(new Gtk::Label("Padstack:"));
			tbox->pack_start(*la, false, false, 0);

			padstack_button = Gtk::manage(new PadstackButton(*core->m_pool, core->get_package()->uuid));
			padstack_button->property_selected_uuid().signal_changed().connect([this]{p_property_can_generate = padstack_button->property_selected_uuid()!=UUID();});
			tbox->pack_start(*padstack_button, false, false, 0);

			box_top->pack_start(*tbox, false, false, 0);
		}


		box_top->show_all();
		box_top->set_margin_top(4);
		box_top->set_margin_start(4);
		box_top->set_margin_end(4);

		overlay = Gtk::manage(new SVGOverlay(resource));
		pack_start(*overlay, true, true, 0);
		overlay->show();
	}
}
