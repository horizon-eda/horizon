#include "preferences_window.hpp"
#include "preferences.hpp"
#include "util/gtk_util.hpp"

namespace horizon {

	class CanvasPreferencesEditor: public Gtk::Grid {
		public:
			CanvasPreferencesEditor(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& x, ImpPreferences *prefs, CanvasPreferences *canvas_prefs);
			static CanvasPreferencesEditor* create(ImpPreferences *prefs, CanvasPreferences *canvas_prefs);
			ImpPreferences *preferences;
			CanvasPreferences *canvas_preferences;
	};

	#define GET_WIDGET(name) do {x->get_widget(#name, name); } while(0)

	CanvasPreferencesEditor::CanvasPreferencesEditor(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& x, ImpPreferences *prefs, CanvasPreferences *canvas_prefs) :
		Gtk::Grid(cobject), preferences(prefs), canvas_preferences(canvas_prefs) {

		Gtk::RadioButton *canvas_bgcolor_blue, *canvas_bgcolor_black, *canvas_grid_style_cross, *canvas_grid_style_dot, *canvas_grid_style_grid;
		GET_WIDGET(canvas_bgcolor_blue);
		GET_WIDGET(canvas_bgcolor_black);
		GET_WIDGET(canvas_grid_style_cross);
		GET_WIDGET(canvas_grid_style_dot);
		GET_WIDGET(canvas_grid_style_grid);
		GET_WIDGET(canvas_grid_style_grid);

		Gtk::Scale *canvas_grid_opacity, *canvas_highlight_dim, *canvas_highlight_shadow, *canvas_highlight_lighten;
		GET_WIDGET(canvas_grid_opacity);
		GET_WIDGET(canvas_highlight_dim);
		GET_WIDGET(canvas_highlight_shadow);
		GET_WIDGET(canvas_highlight_lighten);

		Gtk::Label *canvas_label;
		GET_WIDGET(canvas_label);

		if(canvas_preferences == &preferences->canvas_layer) {
			canvas_label->set_text("Affects Padstack, Package and Board");
		}
		else {
			canvas_label->set_text("Affects Symbol and Schematic");
		}

		std::map<CanvasPreferences::BackgroundColor, Gtk::RadioButton*> bgcolor_widgets = {
			{CanvasPreferences::BackgroundColor::BLUE, canvas_bgcolor_blue},
			{CanvasPreferences::BackgroundColor::BLACK, canvas_bgcolor_black}
		};

		std::map<horizon::Grid::Style, Gtk::RadioButton*> grid_style_widgets = {
			{horizon::Grid::Style::CROSS, canvas_grid_style_cross},
			{horizon::Grid::Style::DOT, canvas_grid_style_dot},
			{horizon::Grid::Style::GRID, canvas_grid_style_grid},
		};

		bind_widget(bgcolor_widgets, canvas_preferences->background_color);
		bind_widget(grid_style_widgets, canvas_preferences->grid_style);
		bind_widget(canvas_grid_opacity, canvas_preferences->grid_opacity);
		bind_widget(canvas_highlight_dim, canvas_preferences->highlight_dim);
		bind_widget(canvas_highlight_shadow, canvas_preferences->highlight_shadow);
		bind_widget(canvas_highlight_lighten, canvas_preferences->highlight_lighten);

		for(auto &it: grid_style_widgets) {
			it.second->signal_toggled().connect([this] {preferences->signal_changed().emit();});
		}
		for(auto &it: bgcolor_widgets) {
			it.second->signal_toggled().connect([this] {preferences->signal_changed().emit();});
		}
		canvas_grid_opacity->signal_value_changed().connect([this]{preferences->signal_changed().emit();});
		canvas_highlight_dim->signal_value_changed().connect([this]{preferences->signal_changed().emit();});
		canvas_highlight_shadow->signal_value_changed().connect([this]{preferences->signal_changed().emit();});
		canvas_highlight_lighten->signal_value_changed().connect([this]{preferences->signal_changed().emit();});
	}

	CanvasPreferencesEditor* CanvasPreferencesEditor::create(ImpPreferences *prefs, CanvasPreferences *canvas_prefs) {
		CanvasPreferencesEditor* w;
		Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
		std::vector<Glib::ustring> widgets = {"canvas_grid", "adjustment1", "adjustment2", "adjustment3", "adjustment4"};
		x->add_from_resource("/net/carrotIndustries/horizon/imp/preferences.ui", widgets);
		x->get_widget_derived("canvas_grid", w, prefs, canvas_prefs);
		w->reference();
		return w;
	}

	ImpPreferencesWindow::ImpPreferencesWindow(ImpPreferences *prefs): Gtk::Window(), preferences(prefs) {
		set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
		auto header = Gtk::manage(new Gtk::HeaderBar());
		header->set_show_close_button(true);
		header->set_title("Preferences");
		set_titlebar(*header);
		header->show();

		auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 0));
		auto sidebar = Gtk::manage(new Gtk::StackSidebar);
		box->pack_start(*sidebar, false, false, 0);
		sidebar->show();

		auto stack = Gtk::manage(new Gtk::Stack);
		sidebar->set_stack(*stack);
		box->pack_start(*stack, true, true, 0);
		stack->show();

		{
			auto ed = CanvasPreferencesEditor::create(preferences, &preferences->canvas_layer);
			stack->add(*ed, "layer", "Layer canvas");
			ed->show();
			ed->unreference();
		}
		{
			auto ed = CanvasPreferencesEditor::create(preferences, &preferences->canvas_non_layer);
			stack->add(*ed, "non-layer", "Non-layer canvas");
			ed->show();
			ed->unreference();
		}

		box->show();
		add(*box);
	}
}
