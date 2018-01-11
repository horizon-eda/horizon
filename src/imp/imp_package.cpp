#include "imp_package.hpp"
#include "pool/part.hpp"
#include "footprint_generator/footprint_generator_window.hpp"
#include "parameter_window.hpp"
#include "header_button.hpp"
#include "widgets/chooser_buttons.hpp"
#include "widgets/pool_browser.hpp"
#include "canvas/canvas_gl.hpp"
#include "3d_view.hpp"
#include "board/board_layers.hpp"
#include "util/gtk_util.hpp"

namespace horizon {
	ImpPackage::ImpPackage(const std::string &package_filename, const std::string &pool_path):
			ImpLayer(pool_path),
			core_package(package_filename, *pool), fake_block(UUID::random()), fake_board(UUID::random(), fake_block) {
		core = &core_package;
		core_package.signal_tool_changed().connect(sigc::mem_fun(this, &ImpBase::handle_tool_change));


		key_seq_append_default(key_seq);
		key_seq.append_sequence({
				{{GDK_KEY_p, GDK_KEY_j}, 	ToolID::PLACE_JUNCTION},
				{{GDK_KEY_j},				ToolID::PLACE_JUNCTION},
				{{GDK_KEY_d, GDK_KEY_l}, 	ToolID::DRAW_LINE},
				{{GDK_KEY_l},				ToolID::DRAW_LINE},
				{{GDK_KEY_d, GDK_KEY_a}, 	ToolID::DRAW_ARC},
				{{GDK_KEY_a},				ToolID::DRAW_ARC},
				{{GDK_KEY_d, GDK_KEY_y}, 	ToolID::DRAW_POLYGON},
				{{GDK_KEY_y}, 				ToolID::DRAW_POLYGON},
				{{GDK_KEY_d, GDK_KEY_r}, 	ToolID::DRAW_POLYGON_RECTANGLE},
				{{GDK_KEY_p, GDK_KEY_t},	ToolID::PLACE_TEXT},
				{{GDK_KEY_t},				ToolID::PLACE_TEXT},
				{{GDK_KEY_p, GDK_KEY_p},	ToolID::PLACE_PAD},
				{{GDK_KEY_P},				ToolID::PLACE_PAD},
				{{GDK_KEY_i},				ToolID::EDIT_PAD_PARAMETER_SET},
		});
		key_seq.signal_update_hint().connect([this] (const std::string &s) {main_window->tool_hint_label->set_text(s);});

	}

	void ImpPackage::canvas_update() {
		canvas->update(*core_package.get_canvas_data());

	}

	void ImpPackage::construct() {
		ImpLayer::construct_layer_box();

		main_window->set_title("Package - Interactive Manipulator");
		state_store = std::make_unique<WindowStateStore>(main_window, "imp-package");

		auto view_3d_button = Gtk::manage(new Gtk::Button("3D"));
		main_window->header->pack_start(*view_3d_button);
		view_3d_button->show();
		view_3d_button->signal_clicked().connect([this]{
			view_3d_window->update(); view_3d_window->present();
		});

		fake_board.set_n_inner_layers(0);
		fake_board.stackup.at(0).substrate_thickness = 1.6_mm;

		view_3d_window = View3DWindow::create(&fake_board, pool.get());
		view_3d_window->from_pool = false;
		view_3d_window->signal_request_update().connect([this] {
			fake_board.polygons.clear();
			{
				auto uu = UUID::random();
				auto &poly = fake_board.polygons.emplace(uu, uu).first->second;
				poly.layer = BoardLayers::L_OUTLINE;

				auto bb = core_package.get_package()->get_bbox();
				bb.first -= Coordi(5_mm, 5_mm);
				bb.second += Coordi(5_mm, 5_mm);

				poly.vertices.emplace_back(bb.first);
				poly.vertices.emplace_back(Coordi(bb.first.x, bb.second.y));
				poly.vertices.emplace_back(bb.second);
				poly.vertices.emplace_back(Coordi(bb.second.x, bb.first.y));
			}

			fake_board.packages.clear();
			{
				auto uu = UUID::random();
				auto &fake_package = fake_board.packages.emplace(uu, uu).first->second;
				fake_package.package = *core_package.get_package();
				fake_package.pool_package = core_package.get_package();
			}
		});



		auto box_3d_filename = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
		box_3d_filename->get_style_context()->add_class("linked");
		auto entry_3d_filename = Gtk::manage(new Gtk::Entry);
		entry_3d_filename->set_width_chars(40);
		bind_widget(entry_3d_filename, core_package.get_package()->model_filename);
		box_3d_filename->pack_start(*entry_3d_filename, true, true, 0);

		{
			auto button = Gtk::manage(new Gtk::Button("Browse..."));
			button->signal_clicked().connect([this, entry_3d_filename] {
				GtkFileChooserNative *native = gtk_file_chooser_native_new ("Open",
					GTK_WINDOW(view_3d_window->gobj()),
					GTK_FILE_CHOOSER_ACTION_OPEN,
					"_Open",
					"_Cancel");
				auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
				auto filter= Gtk::FileFilter::create();
				filter->set_name("STEP models");
				filter->add_pattern("*.step");
				filter->add_pattern("*.stp");
				chooser->add_filter(filter);
				std::string entrytext = entry_3d_filename->get_text();
				if(entrytext.size()) {
					chooser->set_filename(Glib::build_filename(pool->get_base_path(), entrytext));
				}
				else {
					chooser->set_current_folder(pool->get_base_path());
				}

				if(gtk_native_dialog_run (GTK_NATIVE_DIALOG (native))==GTK_RESPONSE_ACCEPT) {
					auto base_path = Gio::File::create_for_path(pool->get_base_path());
					auto rel = base_path->get_relative_path(chooser->get_file());
					if(rel.size()) {
						entry_3d_filename->set_text(rel);
						view_3d_window->update(true);
					}
					else {
						Gtk::MessageDialog md(*view_3d_window,  "Model has to be in the pool directory", false /* use_markup */, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
						md.run();
					}
				}

			});
			box_3d_filename->pack_start(*button, false, false, 0);
		}

		{
			auto box_model_settings = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 5));
			view_3d_window->add_widget(box_model_settings);
			box_model_settings->property_margin() = 10;
			{
				auto la = Gtk::manage(new Gtk::Label("3D Model:"));
				box_model_settings->pack_start(*la, false, false, 0);
			}
			box_model_settings->pack_start(*box_3d_filename, false, false, 0);

			auto button_reload = Gtk::manage(new Gtk::Button);
			button_reload->set_image_from_icon_name("view-refresh-symbolic", Gtk::ICON_SIZE_BUTTON);
			button_reload->signal_clicked().connect([this] {});
			button_reload->set_tooltip_text("Reload Model");
			button_reload->signal_clicked().connect([this]{
				view_3d_window->update(true);
			});
			box_model_settings->pack_start(*button_reload, false, false, 0);


			box_model_settings->show_all();
		}

		footprint_generator_window = FootprintGeneratorWindow::create(main_window, &core_package);
		footprint_generator_window->signal_generated().connect(sigc::mem_fun(this, &ImpBase::canvas_update_from_pp));

		auto parameter_window = new ParameterWindow(main_window, &core_package.parameter_program_code, &core_package.parameter_set);
		{
			auto button = Gtk::manage(new Gtk::Button("Parameters..."));
			main_window->header->pack_start(*button);
			button->show();
			button->signal_clicked().connect([this, parameter_window]{parameter_window->present();});
		}
		{
			auto button = Gtk::manage(new Gtk::Button("Polygon expand"));
			parameter_window->add_button(button);
			button->signal_clicked().connect([this, parameter_window] {
				auto sel = canvas->get_selection();
				if(sel.size()==1) {
					auto &s = *sel.begin();
					if(s.type == ObjectType::POLYGON_EDGE || s.type == ObjectType::POLYGON_VERTEX) {
						auto poly = core.r->get_polygon(s.uuid);
						if(!poly->has_arcs()) {
							std::stringstream ss;
							ss.imbue(std::locale("C"));
							ss << "expand-polygon [ " << poly->parameter_class << " ";
							for(const auto &it: poly->vertices) {
								ss << it.position.x << " " << it.position.y << " ";
							}
							ss << "]\n";
							parameter_window->insert_text(ss.str());
						}

					}
				}
			});
		}
		{
			auto button = Gtk::manage(new Gtk::Button("Insert courtyard program"));
			parameter_window->add_button(button);
			button->signal_clicked().connect([this, parameter_window] {
				const Polygon *poly = nullptr;
				for(const auto &it: core_package.get_package()->polygons) {
					if(it.second.vertices.size() == 4 && !it.second.has_arcs() && it.second.parameter_class == "courtyard") {
						poly = &it.second;
						break;
					}
				}
				if(poly) {
					parameter_window->set_error_message("");
					Coordi a = poly->vertices.at(0).position;
					Coordi b = a;
					for(const auto &v: poly->vertices) {
						a = Coordi::min(a, v.position);
						b = Coordi::max(b, v.position);
					}
					auto c = (a+b)/2;
					auto d = b-a;
					std::stringstream ss;
					ss.imbue(std::locale("C"));
					ss << std::fixed << std::setprecision(3);
					ss << d.x/1e6 << "mm " << d.y/1e6 << "mm\n";
					ss << "get-parameter [ courtyard_expansion ]\n2 * +xy\nset-polygon [ courtyard rectangle ";
					ss << c.x/1e6 << "mm " << c.y/1e6 << "mm ]";
					parameter_window->insert_text(ss.str());
				}
				else {
					parameter_window->set_error_message("no courtyard polygon found: needs to have 4 vertices and the parameter class 'courtyard'");
				}
			});
		}
		parameter_window->signal_apply().connect([this, parameter_window] {
			if(core.r->tool_is_active())
				return;
			auto ps = core_package.get_package(false);
			auto r_compile = ps->parameter_program.set_code(core_package.parameter_program_code);
			if(r_compile.first) {
				parameter_window->set_error_message("<b>Compile error:</b>"+r_compile.second);
				return;
			}
			else {
				parameter_window->set_error_message("");
			}
			ps->parameter_set = core_package.parameter_set;
			auto r = ps->parameter_program.run(ps->parameter_set);
			if(r.first) {
				parameter_window->set_error_message("<b>Run error:</b>"+r.second);
				return;
			}
			else {
				parameter_window->set_error_message("");
			}
			core_package.rebuild();
			canvas_update();
		});

		auto header_button = Gtk::manage(new HeaderButton);
		header_button->set_label(core_package.get_package(false)->name);
		main_window->header->set_custom_title(*header_button);
		header_button->show();

		auto entry_name = header_button->add_entry("Name");
		auto entry_manufacturer = header_button->add_entry("Manufacturer");
		auto entry_tags = header_button->add_entry("Tags");




		auto browser_alt_button = Gtk::manage(new PoolBrowserButton(ObjectType::PACKAGE, pool.get()));
		browser_alt_button->get_browser()->set_show_none(true);
		header_button->add_widget("Alternate for", browser_alt_button);

		entry_name->signal_changed().connect([entry_name, header_button] {header_button->set_label(entry_name->get_text());});

		{
			auto pkg = core_package.get_package(false);
			entry_name->set_text(pkg->name);
			entry_manufacturer->set_text(pkg->manufacturer);
			std::stringstream s;
			std::copy(pkg->tags.begin(), pkg->tags.end(), std::ostream_iterator<std::string>(s, " "));
			entry_tags->set_text(s.str());
			if(pkg->alternate_for)
				browser_alt_button->property_selected_uuid() = pkg->alternate_for->uuid;
		}

		auto hamburger_menu = add_hamburger_menu();
		hamburger_menu->append("Import DXF", "win.import_dxf");

		add_tool_action(ToolID::IMPORT_DXF, "import_dxf");

		{
			auto button = Gtk::manage(new Gtk::Button("Footprint gen."));
			button->signal_clicked().connect([this]{
				footprint_generator_window->present();
				footprint_generator_window->show_all();
			});
			button->show();
			core.r->signal_tool_changed().connect([button](ToolID t){button->set_sensitive(t==ToolID::NONE);});
			main_window->header->pack_end(*button);
		}

		core_package.signal_save().connect([this, entry_name, entry_manufacturer, entry_tags, header_button, browser_alt_button, entry_3d_filename]{
			auto pkg = core_package.get_package(false);
			std::stringstream ss(entry_tags->get_text());
			std::istream_iterator<std::string> begin(ss);
			std::istream_iterator<std::string> end;
			std::vector<std::string> tags(begin, end);
			pkg->tags.clear();
			pkg->tags.insert(tags.begin(), tags.end());
			pkg->name = entry_name->get_text();
			pkg->manufacturer = entry_manufacturer->get_text();
			UUID alt_uuid = browser_alt_button->property_selected_uuid();
			if(alt_uuid) {
				pkg->alternate_for = pool->get_package(alt_uuid);
			}
			else {
				pkg->alternate_for = nullptr;
			}
			header_button->set_label(pkg->name);
		});


	}

	ToolID ImpPackage::handle_key(guint k) {
		return key_seq.handle_key(k);
	}
}
