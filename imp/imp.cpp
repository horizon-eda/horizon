#include "imp.hpp"
#include <gtkmm.h>
#include "block.hpp"
#include "pool_cached.hpp"
#include "core/core_board.hpp"
#include "core/tool_catalog.hpp"
#include "widgets/spin_button_dim.hpp"
#include "export_gerber/gerber_export.hpp"
#include "part.hpp"
#include "rules/rules_window.hpp"
#include "util.hpp"
#include "gtk_util.hpp"
#include "preferences_window.hpp"
#include "property_panels/property_panels.hpp"
#include "widgets/log_window.hpp"
#include "widgets/log_view.hpp"
#include "logger/logger.hpp"
#include "canvas/canvas_gl.hpp"
#include <iomanip>
#include <glibmm/main.h>

namespace horizon {

	std::unique_ptr<Pool> make_pool(const PoolParams &p) {
		if(p.cache_path.size() && Glib::file_test(p.cache_path, Glib::FILE_TEST_IS_DIR)) {
			return std::make_unique<PoolCached>(p.base_path, p.cache_path);
		}
		else {
			return std::make_unique<Pool>(p.base_path);
		}
	}
	
	ImpBase::ImpBase(const PoolParams &params):
		pool(make_pool(params)),
		core(nullptr),
		sock_broadcast_rx(zctx, ZMQ_SUB),
		sock_project(zctx, ZMQ_REQ)
	{
		auto ep_broadcast = Glib::getenv("HORIZON_EP_BROADCAST");
		if(ep_broadcast.size()) {
			sock_broadcast_rx.connect(ep_broadcast);
			{
				unsigned int prefix = 0;
				sock_broadcast_rx.setsockopt(ZMQ_SUBSCRIBE, &prefix, 4);
				prefix = getpid();
				sock_broadcast_rx.setsockopt(ZMQ_SUBSCRIBE, &prefix, 4);
			}
			Glib::RefPtr<Glib::IOChannel> chan;
			#ifdef G_OS_WIN32
				SOCKET fd = sock_broadcast_rx.getsockopt<SOCKET>(ZMQ_FD);
				chan = Glib::IOChannel::create_from_win32_socket(fd);
			#else
				int fd = sock_broadcast_rx.getsockopt<int>(ZMQ_FD);
				chan = Glib::IOChannel::create_from_fd(fd);
			#endif

			Glib::signal_io().connect([this](Glib::IOCondition cond){
				while(sock_broadcast_rx.getsockopt<int>(ZMQ_EVENTS) & ZMQ_POLLIN) {
					zmq::message_t msg;
					sock_broadcast_rx.recv(&msg);
					int prefix;
					memcpy(&prefix, msg.data(), 4);
					char *data = ((char*)msg.data())+4;
					json j = json::parse(data);
					if(prefix == 0 || prefix==getpid()) {
						handle_broadcast(j);
					}

				}
				return true;
			}, chan, Glib::IO_IN | Glib::IO_HUP);
		}
		auto ep_project = Glib::getenv("HORIZON_EP_PROJECT");
		if(ep_project.size()) {
			sock_project.connect(ep_project);
		}
		sockets_connected = ep_project.size() && ep_broadcast.size();
	}
	
	json ImpBase::send_json(const json &j) {
		if(!sockets_connected)
			return nullptr;

		std::string s = j.dump();
		zmq::message_t msg(s.size()+1);
		memcpy(((uint8_t*)msg.data()), s.c_str(), s.size());
		auto m = (char*)msg.data();
		m[msg.size()-1] = 0;
		sock_project.send(msg);

		zmq::message_t rx;
		sock_project.recv(&rx);
		char *rxdata = ((char*)rx.data());
		std::cout << "imp rx " << rxdata << std::endl;
		return json::parse(rxdata);
	}

	bool ImpBase::handle_close(GdkEventAny *ev) {
		bool dontask = false;
		Glib::getenv("HORIZON_NOEXITCONFIRM", dontask);
		if(dontask)
			return false;

		if(!core.r->get_needs_save()) {
			if(preferences_window)
				preferences_window->hide();
			return false;
		}

		Gtk::MessageDialog md(*main_window,  "Save changes before closing?", false /* use_markup */, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_NONE);
		md.set_secondary_text("If you don't save, all your changes will be permanently lost.");
		md.add_button("Close without Saving", 1);
		md.add_button("Cancel", Gtk::RESPONSE_CANCEL);
		md.add_button("Save", 2);
		switch(md.run()) {
			case 1:
				if(preferences_window)
					preferences_window->hide();
				return false; //close

			case 2:
				core.r->save();
				return false; //close

			default:
				return true; //keep window open
		}
		return false;
	}

	void ImpBase::show_preferences_window() {
		if(!preferences_window) {
			preferences.load();
			preferences_window = new ImpPreferencesWindow(&preferences);
			preferences_window->set_transient_for(*main_window);

			preferences_window->signal_hide().connect([this] {
				delete preferences_window;
				preferences_window = nullptr;
				preferences.save();
				preferences.unlock();
			});
		}
		preferences_window->present();
	}

	#define GET_WIDGET(name) do {main_window->builder->get_widget(#name, name); } while(0)

	void ImpBase::run(int argc, char *argv[]) {
		auto app = Gtk::Application::create(argc, argv, "net.carrotIndustries.horizon.Imp", Gio::APPLICATION_NON_UNIQUE);

		main_window = MainWindow::create();
		canvas = main_window->canvas;
		clipboard.reset(new ClipboardManager(core.r));

		canvas->signal_selection_changed().connect(sigc::mem_fun(this, &ImpBase::sc));
		canvas->signal_key_press_event().connect(sigc::mem_fun(this, &ImpBase::handle_key_press));
		canvas->signal_cursor_moved().connect(sigc::mem_fun(this, &ImpBase::handle_cursor_move));
		canvas->signal_button_press_event().connect(sigc::mem_fun(this, &ImpBase::handle_click));
		canvas->signal_request_display_name().connect([this](ObjectType ty, UUID uu){
			return core.r->get_display_name(ty, uu);
		});

		{
			Gtk::RadioButton *selection_tool_box_button, *selection_tool_lasso_button, *selection_tool_paint_button;
			Gtk::RadioButton *selection_qualifier_include_origin_button, *selection_qualifier_touch_box_button, *selection_qualifier_include_box_button;
			GET_WIDGET(selection_tool_box_button);
			GET_WIDGET(selection_tool_lasso_button);
			GET_WIDGET(selection_tool_paint_button);
			GET_WIDGET(selection_qualifier_include_origin_button);
			GET_WIDGET(selection_qualifier_touch_box_button);
			GET_WIDGET(selection_qualifier_include_box_button);

			Gtk::Box *selection_qualifier_box;
			GET_WIDGET(selection_qualifier_box);

			std::map<CanvasGL::SelectionQualifier, Gtk::RadioButton*> qual_map = {
				{CanvasGL::SelectionQualifier::INCLUDE_BOX, selection_qualifier_include_box_button},
				{CanvasGL::SelectionQualifier::TOUCH_BOX, selection_qualifier_touch_box_button},
				{CanvasGL::SelectionQualifier::INCLUDE_ORIGIN, selection_qualifier_include_origin_button}
			};
			bind_widget(qual_map, canvas->selection_qualifier);

			std::map<CanvasGL::SelectionTool, Gtk::RadioButton*> tool_map = {
				{CanvasGL::SelectionTool::BOX, selection_tool_box_button},
				{CanvasGL::SelectionTool::LASSO, selection_tool_lasso_button},
				{CanvasGL::SelectionTool::PAINT, selection_tool_paint_button},
			};
			bind_widget(tool_map, canvas->selection_tool);

			selection_tool_paint_button->signal_toggled().connect([this, selection_tool_paint_button, selection_qualifier_box, selection_qualifier_touch_box_button] {
				auto is_paint = selection_tool_paint_button->get_active();
				if(is_paint) {
					selection_qualifier_touch_box_button->set_active(true);
				}
				selection_qualifier_box->set_sensitive(!is_paint);
			});

		}


		panels = Gtk::manage(new PropertyPanels(core.r));
		panels->show_all();
		main_window->property_viewport->add(*panels);
		panels->signal_update().connect([this] {
			canvas_update();
			canvas->set_selection(panels->get_selection(), false);
		});
		panels->signal_throttled().connect([this] (bool thr) {
			main_window->property_throttled_revealer->set_reveal_child(thr);
		});

		warnings_box = Gtk::manage(new WarningsBox());
		warnings_box->signal_selected().connect(sigc::mem_fun(this, &ImpBase::handle_warning_selected));
		main_window->left_panel->pack_end(*warnings_box, false, false, 0);

		selection_filter_dialog = std::make_unique<SelectionFilterDialog>(this->main_window, &canvas->selection_filter, core.r);

		key_sequence_dialog  = std::make_unique<KeySequenceDialog>(this->main_window);
		key_sequence_dialog->add_sequence(std::vector<unsigned int>{GDK_KEY_Page_Up}, "Layer up");
		key_sequence_dialog->add_sequence(std::vector<unsigned int>{GDK_KEY_Page_Down}, "Layer down");
		key_sequence_dialog->add_sequence(std::vector<unsigned int>{GDK_KEY_space}, "Begin tool");
		key_sequence_dialog->add_sequence(std::vector<unsigned int>{GDK_KEY_Home}, "Zoom all");
		key_sequence_dialog->add_sequence("Ctrl+Z", "Undo");
		key_sequence_dialog->add_sequence("Ctrl+Y", "Redo");
		key_sequence_dialog->add_sequence("Ctrl+C", "Copy");
		key_sequence_dialog->add_sequence("Ctrl+V", "Paste");
		key_sequence_dialog->add_sequence("Ctrl+D", "Duplicate");
		key_sequence_dialog->add_sequence("Ctrl+I", "Selection filter");
		key_sequence_dialog->add_sequence("Esc", "Hover select");
		key_sequence_dialog->add_sequence("Alt (hold)", "Fine grid");

		grid_spin_button = Gtk::manage(new SpinButtonDim());
		grid_spin_button->set_range(0.1_mm, 10_mm);
		grid_spacing_binding = Glib::Binding::bind_property(grid_spin_button->property_value(), canvas->property_grid_spacing(), Glib::BINDING_BIDIRECTIONAL);
		grid_spin_button->set_value(1.25_mm);
		grid_spin_button->show_all();
		main_window->grid_box->pack_start(*grid_spin_button, true, true, 0);

		auto save_button = Gtk::manage(new Gtk::Button("Save"));
		save_button->signal_clicked().connect([this]{core.r->save();});
		save_button->show();
		main_window->header->pack_start(*save_button);

		auto selection_filter_button = Gtk::manage(new Gtk::Button("Selection filter"));
		selection_filter_button->signal_clicked().connect([this]{selection_filter_dialog->show();});
		selection_filter_button->show();
		main_window->header->pack_start(*selection_filter_button);

		auto help_button = Gtk::manage(new Gtk::Button("Help"));
		help_button->signal_clicked().connect([this]{key_sequence_dialog->show();});
		help_button->show();
		main_window->header->pack_end(*help_button);

		if(core.r->get_rules()) {
			rules_window = RulesWindow::create(main_window, canvas, core.r->get_rules(), core.r);
			rules_window->signal_canvas_update().connect(sigc::mem_fun(this, &ImpBase::canvas_update_from_pp));

			{
				auto button = Gtk::manage(new Gtk::Button("Rules..."));
				main_window->header->pack_start(*button);
				button->show();
				button->signal_clicked().connect([this]{rules_window->present();});
				core.r->signal_tool_changed().connect([button](ToolID t){button->set_sensitive(t==ToolID::NONE);});

			}
		}

		preferences.signal_changed().connect(sigc::mem_fun(this, &ImpBase::apply_settings));

		preferences.load();

		preferences_monitor = Gio::File::create_for_path(ImpPreferences::get_preferences_filename())->monitor();

		preferences_monitor->signal_changed().connect([this](const Glib::RefPtr<Gio::File> &file,const Glib::RefPtr<Gio::File> &file_other, Gio::FileMonitorEvent ev){
			if(ev == Gio::FILE_MONITOR_EVENT_CHANGES_DONE_HINT)
				preferences.load();
		});

		main_window->add_action("preferences", [this] {
			if(preferences.lock()) {
				show_preferences_window();
			}
			else {
				Gtk::MessageDialog md(*main_window,  "Can't lock preferences", false /* use_markup */, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_NONE);
				md.add_button("OK", Gtk::RESPONSE_OK);
				md.add_button("Force unlock", 1);
				md.set_secondary_text("Close all other preferences dialogs first");
				if(md.run() == 1) {
					preferences.unlock();
					preferences.lock();
					show_preferences_window();
				}
			}
		});

		log_window = new LogWindow(main_window);
		Logger::get().set_log_handler([this](const Logger::Item &it) {
			log_window->get_view()->push_log(it);
		});

		main_window->add_action("view_log", [this] {
			log_window->present();
		});


		core.r->signal_tool_changed().connect([save_button, selection_filter_button](ToolID t){save_button->set_sensitive(t==ToolID::NONE); selection_filter_button->set_sensitive(t==ToolID::NONE);});

		main_window->signal_delete_event().connect(sigc::mem_fun(this, &ImpBase::handle_close));

		for(const auto &la: core.r->get_layer_provider()->get_layers()) {
			canvas->set_layer_display(la.first, LayerDisplay(true, LayerDisplay::Mode::FILL, la.second.color));
		}

		construct();
		for(const auto &it: key_seq.get_sequences()) {
			key_sequence_dialog->add_sequence(it.keys, tool_catalog.at(it.tool_id).name);
		}

		tool_popover = Gtk::manage(new ToolPopover(canvas, &key_seq));
		tool_popover->set_position(Gtk::POS_BOTTOM);
		tool_popover->signal_tool_activated().connect([this](ToolID tool_id){
			ToolArgs args;
			args.coords = canvas->get_cursor_pos();
			args.selection = canvas->get_selection();
			args.work_layer = canvas->property_work_layer();
			ToolResponse r= core.r->tool_begin(tool_id, args, imp_interface.get());
			tool_process(r);
		});

		canvas->property_work_layer().signal_changed().connect([this]{
			if(core.r->tool_is_active()) {
				ToolArgs args;
				args.type = ToolEventType::LAYER_CHANGE;
				args.coords = canvas->get_cursor_pos();
				args.work_layer = canvas->property_work_layer();
				ToolResponse r = core.r->tool_update(args);
				tool_process(r);
			}
		});

		canvas->signal_grid_mul_changed().connect([this] (unsigned int mul) {
			main_window->grid_mul_label->set_text("×"+std::to_string(mul));
		});

		context_menu = Gtk::manage(new Gtk::Menu());


		imp_interface = std::make_unique<ImpInterface>(this);

		canvas_update();

		auto bbox = core.r->get_bbox();
		canvas->zoom_to_bbox(bbox.first, bbox.second);

		handle_cursor_move(Coordi()); //fixes label

		Gtk::IconTheme::get_default()->add_resource_path("/net/carrotIndustries/horizon/icons");

		app->signal_startup().connect([this, app] {
			auto refBuilder = Gtk::Builder::create();
			refBuilder->add_from_resource("/net/carrotIndustries/horizon/imp/app_menu.ui");

			auto object = refBuilder->get_object("appmenu");
			auto app_menu = Glib::RefPtr<Gio::MenuModel>::cast_dynamic(object);
			app->set_app_menu(app_menu);
			app->add_window(*main_window);
			app->add_action("quit", [this]{
				main_window->close();
			});
		});


		auto cssp = Gtk::CssProvider::create();
		cssp->load_from_data(".imp-status-menu-button {"
			"padding: 1px 8px 2px 4px;\n"
			"border: 0;\n"
			"outline-width: 0;\n"
		"}\n"
		".imp-statusbar {border-top: 1px solid @borders; padding-left:10px;}");
		Gtk::StyleContext::add_provider_for_screen(Gdk::Screen::get_default(), cssp, 700);


		sc();


		app->run(*main_window);
	}
	
	void ImpBase::apply_settings() {
		auto canvas_prefs = get_canvas_preferences();
		if(canvas_prefs->background_color == CanvasPreferences::BackgroundColor::BLUE) {
			canvas->set_background_color(Color::new_from_int(0, 24, 64));
			canvas->set_grid_color(Color::new_from_int(0, 78, 208));
		}
		else {
			canvas->set_grid_color({1,1,1});
			canvas->set_background_color({0,0,0});
		}
		canvas->set_grid_style(canvas_prefs->grid_style);
		canvas->set_grid_alpha(canvas_prefs->grid_opacity);
		canvas->set_highlight_dim(canvas_prefs->highlight_dim);
		canvas->set_highlight_shadow(canvas_prefs->highlight_shadow);
		canvas->set_highlight_lighten(canvas_prefs->highlight_lighten);
		switch(canvas_prefs->grid_fine_modifier) {
			case CanvasPreferences::GridFineModifier::ALT :
				canvas->grid_fine_modifier = Gdk::MOD1_MASK;
			break;
			case CanvasPreferences::GridFineModifier::CTRL :
				canvas->grid_fine_modifier = Gdk::CONTROL_MASK;
			break;
		}
		canvas->show_all_junctions_in_schematic = preferences.schematic.show_all_junctions;
	}

	void ImpBase::canvas_update_from_pp() {
		auto sel = canvas->get_selection();
		canvas_update();
		canvas->set_selection(sel);
	}

	void ImpBase::tool_begin(ToolID id) {
		ToolArgs args;
		args.coords = canvas->get_cursor_pos();
		args.selection = canvas->get_selection();
		args.work_layer = canvas->property_work_layer();
		ToolResponse r= core.r->tool_begin(id, args, imp_interface.get());
		tool_process(r);
	}

	void ImpBase::add_tool_button(ToolID id, const std::string &label, bool left) {
		auto button = Gtk::manage(new Gtk::Button(label));
		button->signal_clicked().connect([this, id]{
			tool_begin(id);
		});
		button->show();
		core.r->signal_tool_changed().connect([button](ToolID t){button->set_sensitive(t==ToolID::NONE);});
		if(left)
			main_window->header->pack_start(*button);
		else
			main_window->header->pack_end(*button);
	}

	void ImpBase::add_tool_action(ToolID tid, const std::string &action) {
		auto tool_action = main_window->add_action(action, [this, tid] {
			tool_begin(tid);
		});
	}

	Glib::RefPtr<Gio::Menu> ImpBase::add_hamburger_menu() {
		auto hamburger_button = Gtk::manage(new Gtk::MenuButton);
		hamburger_button->set_image_from_icon_name("open-menu-symbolic", Gtk::ICON_SIZE_BUTTON);
		core.r->signal_tool_changed().connect([hamburger_button](ToolID t){hamburger_button->set_sensitive(t==ToolID::NONE);});

		auto hamburger_menu = Gio::Menu::create();
		hamburger_button->set_menu_model(hamburger_menu);
		hamburger_button->show();
		main_window->header->pack_end(*hamburger_button);

		return hamburger_menu;
	}

	bool ImpBase::handle_key_press(GdkEventKey *key_event) {
		bool is_layer_change = (key_event->keyval == GDK_KEY_Page_Up) || (key_event->keyval == GDK_KEY_Page_Down) || (key_event->keyval >= GDK_KEY_0 && key_event->keyval <= GDK_KEY_9);
		if(is_layer_change) {
			if((key_event->keyval == GDK_KEY_Page_Up) || (key_event->keyval == GDK_KEY_Page_Down)) {
				int wl = canvas->property_work_layer();
				auto layers = core.r->get_layer_provider()->get_layers();
				std::vector<int> layer_indexes;
				layer_indexes.reserve(layers.size());
				std::transform(layers.begin(), layers.end(), std::back_inserter(layer_indexes), [](const auto &x){return x.first;});

				int idx = std::find(layer_indexes.begin(), layer_indexes.end(), wl) - layer_indexes.begin();
				if(key_event->keyval == GDK_KEY_Page_Up) {
					idx++;
				}
				else {
					idx--;
				}
				if(idx>=0 && idx < (int)layers.size()) {
					canvas->property_work_layer() = layer_indexes.at(idx);
				}
			}
			else if(key_event->keyval >= GDK_KEY_0 && key_event->keyval <= GDK_KEY_9) {
				int n = key_event->keyval - GDK_KEY_0;
				int layer = 0;
				if(n == 1) {
					layer = 0;
				}
				else if(n == 2) {
					layer = -100;
				}
				else {
					layer = -(n-2);
				}
				if(core.r->get_layer_provider()->get_layers().count(layer)) {
					canvas->property_work_layer() = layer;
				}
			}
			if(core.r->tool_is_active()) { //inform tool about layer change
				ToolArgs args;
				args.type = ToolEventType::LAYER_CHANGE;
				args.coords = canvas->get_cursor_pos();
				args.work_layer = canvas->property_work_layer();
				ToolResponse r = core.r->tool_update(args);
				tool_process(r);
			}
			return true;
		}

		if(!core.r->tool_is_active()) {
			if(key_event->keyval == GDK_KEY_Escape) {
				canvas->selection_mode = CanvasGL::SelectionMode::HOVER;
				canvas->set_selection({});
				return true;
			}
			ToolID t = ToolID::NONE;
			if(!(key_event->state & Gdk::ModifierType::CONTROL_MASK)) {
				t = handle_key(key_event->keyval);
			}

			if(t != ToolID::NONE) {
				tool_begin(t);
				return true;
			}
			else {
				if((key_event->keyval == GDK_KEY_space)) {
					Gdk::Rectangle rect;
					auto c = canvas->get_cursor_pos_win();
					rect.set_x(c.x);
					rect.set_y(c.y);
					tool_popover->set_pointing_to(rect);

					std::map<ToolID, bool> can_begin;
					auto sel = canvas->get_selection();
					for(const auto &it: tool_catalog) {
						bool r = core.r->tool_can_begin(it.first, sel).first;
						can_begin[it.first] = r;
					}
					tool_popover->set_can_begin(can_begin);

					#if GTK_CHECK_VERSION(3,22,0)
						tool_popover->popup();
					#else
						tool_popover->show();
					#endif
					return true;
				}
				else if((key_event->keyval == GDK_KEY_Home)) {
					auto bbox = core.r->get_bbox();
					canvas->zoom_to_bbox(bbox.first, bbox.second);
					return true;
				}
				else if(key_event->keyval == GDK_KEY_question) {
					key_sequence_dialog->show();
					return true;
				}

				if(key_event->state & Gdk::ModifierType::CONTROL_MASK) {
					if(key_event->keyval == GDK_KEY_z) {
						std::cout << "undo" << std::endl;
						core.r->undo();
						canvas_update_from_pp();
						return true;
					}
					else if(key_event->keyval == GDK_KEY_y) {
						std::cout << "redo" << std::endl;
						core.r->redo();
						canvas_update_from_pp();
						return true;
					}
					else if(key_event->keyval == GDK_KEY_c) {
						clipboard->copy(canvas->get_selection(), canvas->get_cursor_pos());
						return true;
					}
					else if(key_event->keyval == GDK_KEY_v) {
						tool_begin(ToolID::PASTE);
						return true;
					}
					else if(key_event->keyval == GDK_KEY_d) {
						clipboard->copy(canvas->get_selection(), canvas->get_cursor_pos());
						tool_begin(ToolID::PASTE);
						return true;
					}
					else if(key_event->keyval == GDK_KEY_i) {
						selection_filter_dialog->show();
						return true;
					}
				}

			}
		}
		else {
			ToolArgs args;
			args.coords = canvas->get_cursor_pos();
			args.work_layer = canvas->property_work_layer();
			if(key_event->keyval == GDK_KEY_Escape) {
				args.type = ToolEventType::CLICK;
				args.button = 3;
			}
			else {
				args.type = ToolEventType::KEY;
				args.key = key_event->keyval;
			}

			ToolResponse r = core.r->tool_update(args);
			tool_process(r);
			return true;
		}
		return false;
	}
	
	void ImpBase::handle_cursor_move(const Coordi &pos) {
		if(core.r->tool_is_active()) {
			ToolArgs args;
			args.type = ToolEventType::MOVE;
			args.coords = pos;
			args.work_layer = canvas->property_work_layer();
			ToolResponse r = core.r->tool_update(args);
			tool_process(r);
		}
		main_window->cursor_label->set_text(coord_to_string(pos));
	}
	
	void ImpBase::fix_cursor_pos() {
		auto ev = gtk_get_current_event();
		auto dev = gdk_event_get_device(ev);
		auto win = canvas->get_window()->gobj();
		gdouble x,y;
		gdk_window_get_device_position_double(win, dev, &x, &y, nullptr);
		if(!canvas->get_has_window()) {
			auto alloc = canvas->get_allocation();
			x-=alloc.get_x();
			y-=alloc.get_y();
		}
		canvas->update_cursor_pos(x, y);
	}

	bool ImpBase::handle_click(GdkEventButton *button_event) {
		if(core.r->tool_is_active() && button_event->button != 2 && !(button_event->state & Gdk::SHIFT_MASK)) {
			ToolArgs args;
			args.type = ToolEventType::CLICK;
			args.coords = canvas->get_cursor_pos();
			args.button = button_event->button;
			args.target = canvas->get_current_target();
			args.work_layer = canvas->property_work_layer();
			ToolResponse r = core.r->tool_update(args);
			tool_process(r);
		}
		else if(!core.r->tool_is_active() && button_event->button == 1) {
			handle_maybe_drag();
		}
		else if(!core.r->tool_is_active() && button_event->button == 3) {
			for(const auto it: context_menu->get_children()) {
				delete it;
			}
			std::set<SelectableRef> sel_for_menu;
			if(canvas->selection_mode == CanvasGL::SelectionMode::HOVER) {
				sel_for_menu = canvas->get_selection();

			}
			else {
				auto c = canvas->screen2canvas(Coordf(button_event->x, button_event->y));
				auto sel = canvas->get_selection_at(Coordi(c.x, c.y));
				auto sel_from_canvas = canvas->get_selection();
				std::set<SelectableRef> isect;
				std::set_intersection(sel.begin(), sel.end(), sel_from_canvas.begin(), sel_from_canvas.end(), std::inserter(isect, isect.begin()));
				if(isect.size()) { //was in selection
					sel_for_menu = sel_from_canvas;
				}
				else if(sel.size()==1){ //there's exactly one item
					canvas->set_selection(sel, false);
					sel_for_menu = sel;
				}
				else if(sel.size()>1){ //multiple items: do our own menu
					canvas->set_selection({}, false);
					for(const auto &sr: sel) {
						std::string text = object_descriptions.at(sr.type).name;
						auto display_name = core.r->get_display_name(sr.type, sr.uuid);
						if(display_name.size()) {
							text += " "+display_name;
						}
						auto layers = core.r->get_layer_provider()->get_layers();
						if(layers.count(sr.layer)) {
							text += " ("+layers.at(sr.layer).name+")";
						}
						auto la =  Gtk::manage(new Gtk::MenuItem(text));
						la->signal_select().connect([this, sr] {
							canvas->set_selection({sr}, false);
						});
						la->signal_deselect().connect([this] {
							canvas->set_selection({}, false);
						});
						auto submenu = Gtk::manage(new Gtk::Menu);

						{
							auto la2 =  Gtk::manage(new Gtk::MenuItem("Copy"));
							la2->signal_activate().connect([this, sr] {
								canvas->set_selection({sr}, false);
								clipboard->copy(canvas->get_selection(), canvas->get_cursor_pos());
							});
							la2->show();
							submenu->append(*la2);
						}
						{
							auto la2 =  Gtk::manage(new Gtk::MenuItem("Duplicate"));
							la2->signal_activate().connect([this, sr] {
								canvas->set_selection({sr}, false);
								clipboard->copy(canvas->get_selection(), canvas->get_cursor_pos());
								tool_begin(ToolID::PASTE);
							});
							la2->show();
							submenu->append(*la2);
						}

						{
							auto sep = Gtk::manage(new Gtk::SeparatorMenuItem);
							sep->show();
							submenu->append(*sep);
						}

						for(const auto &it: tool_catalog) {
							auto r = core.r->tool_can_begin(it.first, {sr});
							if(r.first && r.second) {
								auto la_sub =  Gtk::manage(new Gtk::MenuItem(it.second.name));
								ToolID tool_id = it.first;
								la_sub->signal_activate().connect([this, tool_id, sr] {
									canvas->set_selection({sr}, false);
									fix_cursor_pos();
									tool_begin(tool_id);
								});
								la_sub->show();
								submenu->append(*la_sub);
							}
						}
						la->set_submenu(*submenu);
						la->show();
						context_menu->append(*la);
					}
					#if GTK_CHECK_VERSION(3,22,0)
						context_menu->popup_at_pointer((GdkEvent*)button_event);
					#else
						context_menu->popup(0, gtk_get_current_event_time());
					#endif
					sel_for_menu.clear();
				}
			}
			if(sel_for_menu.size()) {

				{
					auto la =  Gtk::manage(new Gtk::MenuItem("Copy"));
					la->signal_activate().connect([this] {
						clipboard->copy(canvas->get_selection(), canvas->get_cursor_pos());
					});
					la->show();
					context_menu->append(*la);
				}
				{
					auto la =  Gtk::manage(new Gtk::MenuItem("Duplicate"));
					la->signal_activate().connect([this] {
						clipboard->copy(canvas->get_selection(), canvas->get_cursor_pos());
						tool_begin(ToolID::PASTE);
					});
					la->show();
					context_menu->append(*la);
				}

				{
					auto sep = Gtk::manage(new Gtk::SeparatorMenuItem);
					sep->show();
					context_menu->append(*sep);
				}

				for(const auto &it: tool_catalog) {
					auto r = core.r->tool_can_begin(it.first, sel_for_menu);
					if(r.first && r.second) {
						auto la =  Gtk::manage(new Gtk::MenuItem(it.second.name));
						ToolID tool_id = it.first;
						la->signal_activate().connect([this, tool_id] {
							fix_cursor_pos();
							tool_begin(tool_id);
						});
						la->show();
						context_menu->append(*la);
					}
				}
				#if GTK_CHECK_VERSION(3,22,0)
					context_menu->popup_at_pointer((GdkEvent*)button_event);
				#else
					context_menu->popup(0, gtk_get_current_event_time());
				#endif
			}

		}
		return false;
	}

	void ImpBase::tool_process(const ToolResponse &resp) {
		if(!core.r->tool_is_active()) {
			main_window->tool_hint_label->set_text(">");
			canvas->set_cursor_external(false);
			no_update = false;
			highlights.clear();
			update_highlights();
		}
		if(!no_update) {
			canvas_update();
			canvas->set_selection(core.r->selection);
		}
		if(resp.layer != 10000) {
			canvas->property_work_layer() = resp.layer;
		}
		if(resp.next_tool != ToolID::NONE) {
			ToolArgs args;
			args.coords = canvas->get_cursor_pos();
			args.keep_selection = true;
			ToolResponse r = core.r->tool_begin(resp.next_tool, args, imp_interface.get());
			tool_process(r);
		}
	}
	
	void ImpBase::sc(void) {
		//std::cout << "Selection changed\n";
		//std::cout << "---" << std::endl;
		if(!core.r->tool_is_active()) {
			highlights.clear();
			update_highlights();

			auto sel = canvas->get_selection();
			decltype(sel) sel_extra;
			for(const auto &it: sel) {
				switch(it.type) {
					case ObjectType::SCHEMATIC_SYMBOL :
						sel_extra.emplace(core.c->get_schematic_symbol(it.uuid)->component->uuid, ObjectType::COMPONENT);
					break;
					case ObjectType::JUNCTION:
						if(core.r->get_junction(it.uuid)->net && core.c) {
							sel_extra.emplace(core.r->get_junction(it.uuid)->net->uuid, ObjectType::NET);
						}
					break;
					case ObjectType::LINE_NET: {
						LineNet &li = core.c->get_sheet()->net_lines.at(it.uuid);
						if(li.net) {
							sel_extra.emplace(li.net->uuid, ObjectType::NET);
						}
					}break;
					case ObjectType::NET_LABEL: {
						NetLabel &la = core.c->get_sheet()->net_labels.at(it.uuid);
						if(la.junction->net) {
							sel_extra.emplace(la.junction->net->uuid, ObjectType::NET);
						}
					}break;
					case ObjectType::POWER_SYMBOL: {
						PowerSymbol &sym = core.c->get_sheet()->power_symbols.at(it.uuid);
						if(sym.net) {
							sel_extra.emplace(sym.net->uuid, ObjectType::NET);
						}
					}break;
					case ObjectType::POLYGON_EDGE:
					case ObjectType::POLYGON_VERTEX: {
						sel_extra.emplace(it.uuid, ObjectType::POLYGON);
						auto poly = core.r->get_polygon(it.uuid);
						if(poly->usage && poly->usage->get_type() == PolygonUsage::Type::PLANE) {
							sel_extra.emplace(poly->usage->get_uuid(), ObjectType::PLANE);
						}
					}break;
					default: ;
				}
			}

			sel.insert(sel_extra.begin(), sel_extra.end());
			panels->update_objects(sel);
			bool show_properties = panels->get_selection().size()>0;
			main_window->property_scrolled_window->set_visible(show_properties);
			main_window->property_throttled_revealer->set_visible(show_properties);
		}
	}

	void ImpBase::handle_tool_change(ToolID id) {
		panels->set_sensitive(id == ToolID::NONE);
		canvas->set_selection_allowed(id == ToolID::NONE);
		if(id!=ToolID::NONE) {
			main_window->tool_bar_set_tool_name(tool_catalog.at(id).name);
			main_window->tool_bar_set_tool_tip("");
		}
		main_window->tool_bar_set_visible(id!=ToolID::NONE);

	}

	void ImpBase::handle_warning_selected(const Coordi &pos) {
		canvas->center_and_zoom(pos);
	}

	bool ImpBase::handle_broadcast(const json &j) {
		std::string op = j.at("op");
		if(op == "present") {
			main_window->present();
			return true;
		}
		else if(op == "save") {
			core.r->save();
			return true;
		}
		return false;
	}

	void ImpBase::key_seq_append_default(KeySequence &ks) {
		ks.append_sequence({
				{{GDK_KEY_m},				ToolID::MOVE},
				{{GDK_KEY_M},				ToolID::MOVE_EXACTLY},
				{{GDK_KEY_Delete}, 			ToolID::DELETE},
				{{GDK_KEY_Return}, 			ToolID::ENTER_DATUM},
				{{GDK_KEY_r}, 				ToolID::ROTATE},
				{{GDK_KEY_e}, 				ToolID::MIRROR},
				{{GDK_KEY_Insert},			ToolID::PASTE},
		});
	}
}
