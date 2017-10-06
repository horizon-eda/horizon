#pragma once
#include <gtkmm.h>
#include "common.hpp"
#include <memory>
#include "editor_interface.hpp"

namespace horizon {
	class EditorWindowStore {
		public:
		EditorWindowStore(const std::string &fn);
		void save();
		virtual void save_as(const std::string &fn) = 0;
		std::string filename;
		virtual ~EditorWindowStore(){}
	};

	class EditorWindow: public Gtk::Window {
		public:
			EditorWindow(ObjectType type, const std::string &filename, class Pool *p);
			void reload();
			bool get_need_update();

		private:
			ObjectType type;
			std::unique_ptr<EditorWindowStore> store = nullptr;
			PoolEditorInterface *iface = nullptr;
			Gtk::Button *save_button = nullptr;
			class Pool *pool;
			void save();
			bool need_update = false;

	};
}
