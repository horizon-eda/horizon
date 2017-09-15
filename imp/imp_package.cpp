#include "imp_package.hpp"
#include "part.hpp"
#include "footprint_generator/footprint_generator_window.hpp"
#include "imp/parameter_window.hpp"

namespace horizon {
	ImpPackage::ImpPackage(const std::string &package_filename, const std::string &pool_path):
			ImpLayer(pool_path),
			core_package(package_filename, pool) {
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

		{
			auto button = Gtk::manage(new Gtk::Button("Footprint gen."));
			main_window->top_panel->pack_start(*button, false, false, 0);
			button->show();
			button->signal_clicked().connect([this]{footprint_generator_window->show_all();});
			core.r->signal_tool_changed().connect([button](ToolID t){button->set_sensitive(t==ToolID::NONE);});
		}

		add_tool_button(ToolID::IMPORT_DXF, "Import DXF");

		footprint_generator_window = FootprintGeneratorWindow::create(main_window, &core_package);
		footprint_generator_window->signal_generated().connect(sigc::mem_fun(this, &ImpBase::canvas_update_from_pp));

		auto parameter_window = new ParameterWindow(main_window, &core_package.parameter_program_code, &core_package.parameter_set);
		{
			auto button = Gtk::manage(new Gtk::Button("Parameters..."));
			main_window->top_panel->pack_start(*button, false, false, 0);
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

		auto meta_button = Gtk::manage(new Gtk::MenuButton());
		main_window->top_panel->pack_start(*meta_button, false, false, 0);
		meta_button->show();
		auto meta_popover = Gtk::manage(new Gtk::Popover(*meta_button));
		meta_button->set_popover(*meta_popover);
		auto gr = Gtk::manage(new Gtk::Grid());
		gr->set_row_spacing(10);
		gr->set_column_spacing(10);
		gr->set_margin_start(20);
		gr->set_margin_end(20);
		gr->set_margin_top(20);
		gr->set_margin_bottom(20);
		auto attach_label = [gr](const std::string &las, int y) {
			auto la = Gtk::manage(new Gtk::Label(las));
			la->set_halign(Gtk::ALIGN_END);
			la->get_style_context()->add_class("dim-label");
			gr->attach(*la, 0, y, 1, 1);
			auto entry = Gtk::manage(new Gtk::Entry());
			gr->attach(*entry, 1, y, 1, 1);
			return entry;
		};
		auto entry_name = attach_label("Name", 0);
		auto entry_manufacturer = attach_label("Manufacturer", 1);
		auto entry_tags = attach_label("Tags", 2);
		gr->show_all();
		meta_popover->add(*gr);

		{
			auto pkg = core_package.get_package(false);
			entry_name->set_text(pkg->name);
			entry_manufacturer->set_text(pkg->manufacturer);
			std::stringstream s;
			std::copy(pkg->tags.begin(), pkg->tags.end(), std::ostream_iterator<std::string>(s, " "));
			entry_tags->set_text(s.str());
		}

		core_package.signal_save().connect([this, entry_name, entry_manufacturer, entry_tags]{
			auto pkg = core_package.get_package(false);
			std::stringstream ss(entry_tags->get_text());
			std::istream_iterator<std::string> begin(ss);
			std::istream_iterator<std::string> end;
			std::vector<std::string> tags(begin, end);
			pkg->tags.clear();
			pkg->tags.insert(tags.begin(), tags.end());
			pkg->name = entry_name->get_text();
			pkg->manufacturer = entry_manufacturer->get_text();
		});


	}

	ToolID ImpPackage::handle_key(guint k) {
		return key_seq.handle_key(k);
	}
}
