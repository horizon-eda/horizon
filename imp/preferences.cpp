#include "preferences.hpp"
#include <glibmm/miscutils.h>
#include <glibmm/fileutils.h>
#include <giomm/file.h>
#include "util.hpp"
#include "lut.hpp"
#include <fstream>

namespace horizon {

	static const LutEnumStr<CanvasPreferences::BackgroundColor> bg_color_lut = {
		{"blue",	CanvasPreferences::BackgroundColor::BLUE},
		{"black",	CanvasPreferences::BackgroundColor::BLACK},
	};

	static const LutEnumStr<Grid::Style> grid_style_lut = {
		{"cross",	Grid::Style::CROSS},
		{"dot",		Grid::Style::DOT},
		{"grid",	Grid::Style::GRID},
	};

	ImpPreferences::ImpPreferences() {

	}

	void ImpPreferences::load_default() {
		canvas_layer = CanvasPreferences();
		canvas_non_layer= CanvasPreferences();
	}

	std::string ImpPreferences::get_preferences_filename() {
		return Glib::build_filename(Glib::get_user_config_dir(), "horizon", "imp-prefs.json");
	}

	static std::string get_preferences_lock_filename() {
		return Glib::build_filename(Glib::get_user_config_dir(), "horizon", "imp-prefs.json.lock");
	}

	json CanvasPreferences::serialize() const {
		json j;
		j["background_color"] = bg_color_lut.lookup_reverse(background_color);
		j["grid_style"] = grid_style_lut.lookup_reverse(grid_style);
		j["grid_opacity"] = grid_opacity;
		j["highlight_shadow"] = highlight_shadow;
		j["highlight_dim"] = highlight_dim;
		j["highlight_lighten"] = highlight_lighten;
		return j;
	}

	void CanvasPreferences::load_from_json(const json &j) {
		background_color = bg_color_lut.lookup(j.at("background_color"));
		grid_style = grid_style_lut.lookup(j.at("grid_style"));
		grid_opacity = j.value("grid_opacity", .4);
		highlight_dim = j.value("highlight_dim", .3);
		highlight_shadow = j.value("highlight_shadow", .3);
		highlight_lighten = j.value("highlight_lighten", .3);
	}

	json ImpPreferences::serialize() const {
		json j;
		j["canvas_layer"] = canvas_layer.serialize();
		j["canvas_non_layer"] = canvas_non_layer.serialize();
		return j;
	}

	void ImpPreferences::save() {
		std::string prefs_filename = get_preferences_filename();
		auto prefs_dir = Glib::path_get_dirname(prefs_filename);
		if(!Glib::file_test(prefs_dir, Glib::FILE_TEST_EXISTS))
			Gio::File::create_for_path(prefs_dir)->make_directory_with_parents();

		json j = serialize();
		save_json_to_file(prefs_filename, j);
	}

	void ImpPreferences::load() {
		std::string prefs_filename = get_preferences_filename();
		if(!Glib::file_test(prefs_filename, Glib::FILE_TEST_IS_REGULAR)) {
			load_default();
		}
		else {
			json j;
			std::ifstream ifs(prefs_filename);
			if(!ifs.is_open()) {
				throw std::runtime_error("file "  +filename+ " not opened");
			}
			ifs>>j;
			ifs.close();

			canvas_layer.load_from_json(j.at("canvas_layer"));
			canvas_non_layer.load_from_json(j.at("canvas_non_layer"));
		}
		s_signal_changed.emit();
	}

	bool ImpPreferences::lock() {
		std::string prefs_lock_filename = get_preferences_lock_filename();
		if(Glib::file_test(prefs_lock_filename, Glib::FILE_TEST_IS_REGULAR)) {
			return false;
		}
		std::ofstream of(prefs_lock_filename);
		of.close();
		return true;
	}

	void ImpPreferences::unlock() {
		std::string prefs_lock_filename = get_preferences_lock_filename();
		if(Glib::file_test(prefs_lock_filename, Glib::FILE_TEST_IS_REGULAR)) {
			Gio::File::create_for_path(prefs_lock_filename)->remove();
		}
	}
}
