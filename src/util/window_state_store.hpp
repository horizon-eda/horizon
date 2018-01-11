#pragma once
#include "util/sqlite.hpp"

namespace Gtk {
	class Window;
}

namespace horizon {
	class WindowState {
		public:
			WindowState(int ax, int ay, int aw, int ah, bool m=false): x(ax), y(ay), width(aw), height(ah), maximized(m) {};
			WindowState() {};
			int x = 0;
			int y = 0;
			int width = 1024;
			int height = 768;
			bool maximized = false;
	};

	class WindowStateStore {
		public:
			WindowStateStore(Gtk::Window *w, const std::string &window_name);
			bool get_default_set() const;

		private:
			SQLite::Database db;
			const std::string window_name;
			Gtk::Window *win = nullptr;
			WindowState window_state;

			bool load(const std::string &win, WindowState &ws);
			void save(const std::string &win, const WindowState &ws);

			void apply(const WindowState &ws);
			bool default_set = false;

	};
}
