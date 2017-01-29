#include "imp_package.hpp"
#include "json.hpp"
#include "part.hpp"
#include "footprint_generator/footprint_generator_window.hpp"

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
		canvas->set_core(core.r);
		ImpLayer::construct();

		{
			auto button = Gtk::manage(new Gtk::Button("Footprint gen."));
			main_window->top_panel->pack_start(*button, false, false, 0);
			button->show();
			button->signal_clicked().connect([this]{footprint_generator_window->show_all();});
			core.r->signal_tool_changed().connect([button](ToolID t){button->set_sensitive(t==ToolID::NONE);});
		}

		auto name_entry = Gtk::manage(new Gtk::Entry());
		name_entry->show();
		main_window->top_panel->pack_start(*name_entry, false, false, 0);
		name_entry->set_text(core_package.get_package(false)->name);
		core_package.signal_save().connect([this, name_entry]{core_package.get_package(false)->name = name_entry->get_text();});

		auto tags_entry = Gtk::manage(new Gtk::Entry());
		tags_entry->show();
		main_window->top_panel->pack_start(*tags_entry, false, false, 0);
		{
			auto pkg = core_package.get_package(false);
			std::stringstream s;
			std::copy(pkg->tags.begin(), pkg->tags.end(), std::ostream_iterator<std::string>(s, " "));
			tags_entry->set_text(s.str());
		}
		core_package.signal_save().connect([this, tags_entry]{
			auto pkg = core_package.get_package(false);
			std::stringstream ss(tags_entry->get_text());
			std::istream_iterator<std::string> begin(ss);
			std::istream_iterator<std::string> end;
			std::vector<std::string> tags(begin, end);
			pkg->tags.clear();
			pkg->tags.insert(tags.begin(), tags.end());
		});

		footprint_generator_window = FootprintGeneratorWindow::create(main_window, &core_package);
		footprint_generator_window->signal_generated().connect(sigc::mem_fun(this, &ImpBase::canvas_update_from_pp));


	}

	ToolID ImpPackage::handle_key(guint k) {
		return key_seq.handle_key(k);
	}
}
