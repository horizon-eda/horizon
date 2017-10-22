#include "3d_view.hpp"
#include "canvas3d/canvas3d.hpp"

namespace horizon {

	View3DWindow* View3DWindow::create(const class Board *b) {
		View3DWindow* w;
		Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
		x->add_from_resource("/net/carrotIndustries/horizon/imp/3d_view.ui");
		x->get_widget_derived("window", w, b);

		return w;
	}

	View3DWindow::View3DWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& x, const class Board *bo) :
		Gtk::Window(cobject), board(bo) {

		Gtk::Box *gl_box;
		x->get_widget("gl_box", gl_box);

		canvas = Gtk::manage(new Canvas3D);
		update();
		gl_box->pack_start(*canvas, true, true, 0);
		canvas->show();

		auto cssp = Gtk::CssProvider::create();
		cssp->load_from_data(".imp-settings-overlay {border-radius:0px;}");
		Gtk::StyleContext::add_provider_for_screen(Gdk::Screen::get_default(), cssp, 700);


		Gtk::Revealer *rev;
		x->get_widget("revealer", rev);
		Gtk::ToggleButton *settings_button;
		x->get_widget("settings_button", settings_button);
		settings_button->signal_toggled().connect([settings_button, rev]{rev->set_reveal_child(settings_button->get_active());});

		Gtk::Button *update_button;
		x->get_widget("update_button", update_button);
		update_button->signal_clicked().connect(sigc::mem_fun(this, &View3DWindow::update));

		auto explode_adj = Glib::RefPtr<Gtk::Adjustment>::cast_dynamic(x->get_object("explode_adj"));
		explode_adj->signal_value_changed().connect([explode_adj, this] {
			canvas->explode = explode_adj->get_value();
			canvas->queue_draw();
		});

		Gtk::ColorButton *solder_mask_color_button;
		x->get_widget("solder_mask_color_button", solder_mask_color_button);
		solder_mask_color_button->property_color().signal_changed().connect([this, solder_mask_color_button] {
			auto co = solder_mask_color_button->get_color();
			canvas->solder_mask_color.r = co.get_red_p();
			canvas->solder_mask_color.g = co.get_green_p();
			canvas->solder_mask_color.b = co.get_blue_p();
			canvas->prepare();
		});
		solder_mask_color_button->set_color(Gdk::Color("#008000"));

		Gtk::Switch *solder_mask_switch;
		x->get_widget("solder_mask_switch", solder_mask_switch);
		solder_mask_switch->property_active().signal_changed().connect([this, solder_mask_switch]{
			canvas->show_solder_mask = solder_mask_switch->get_active();
			canvas->prepare();
		});

		Gtk::Switch *silkscreen_switch;
		x->get_widget("silkscreen_switch", silkscreen_switch);
		silkscreen_switch->property_active().signal_changed().connect([this, silkscreen_switch]{
			canvas->show_silkscreen = silkscreen_switch->get_active();
			canvas->prepare();
		});

		Gtk::Switch *substrate_switch;
		x->get_widget("substrate_switch", substrate_switch);
		substrate_switch->property_active().signal_changed().connect([this, substrate_switch]{
			canvas->show_substrate = substrate_switch->get_active();
			canvas->prepare();
		});

	}

	void View3DWindow::update() {
		canvas->patches.clear();
		canvas->update2(*board);
	}
}
