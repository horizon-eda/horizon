#include "imp_package.hpp"
#include "part.hpp"
#include "footprint_generator/footprint_generator_window.hpp"
#include "imp/parameter_window.hpp"
#include "header_button.hpp"

namespace horizon {
	ImpPackage::ImpPackage(const std::string &package_filename, const std::string &pool_path):
			ImpLayer(pool_path),
			core_package(package_filename, *pool) {
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
		});
		key_seq.signal_update_hint().connect([this] (const std::string &s) {main_window->tool_hint_label->set_text(s);});

	}

	void ImpPackage::canvas_update() {
		canvas->update(*core_package.get_canvas_data());

	}

	void ImpPackage::construct() {
		ImpLayer::construct();

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

		{
			auto pkg = core_package.get_package(false);
			entry_name->set_text(pkg->name);
			entry_manufacturer->set_text(pkg->manufacturer);
			std::stringstream s;
			std::copy(pkg->tags.begin(), pkg->tags.end(), std::ostream_iterator<std::string>(s, " "));
			entry_tags->set_text(s.str());
		}

		auto hamburger_menu = add_hamburger_menu();
		hamburger_menu->append("Import DXF", "win.import_dxf");
		hamburger_menu->append("Footprint generator", "win.footprint_gen");

		add_tool_action(ToolID::IMPORT_DXF, "import_dxf");

		{
			auto tool_action = main_window->add_action("footprint_gen");
			tool_action->signal_activate().connect([this](const Glib::VariantBase& var){
				footprint_generator_window->show_all();
			});
		}

		core_package.signal_save().connect([this, entry_name, entry_manufacturer, entry_tags, header_button]{
			auto pkg = core_package.get_package(false);
			std::stringstream ss(entry_tags->get_text());
			std::istream_iterator<std::string> begin(ss);
			std::istream_iterator<std::string> end;
			std::vector<std::string> tags(begin, end);
			pkg->tags.clear();
			pkg->tags.insert(tags.begin(), tags.end());
			pkg->name = entry_name->get_text();
			pkg->manufacturer = entry_manufacturer->get_text();
			header_button->set_label(pkg->name);
		});


	}

	ToolID ImpPackage::handle_key(guint k) {
		return key_seq.handle_key(k);
	}
}
