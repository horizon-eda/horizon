#pragma once
#include <gtkmm.h>
#include <memory>
#include "uuid.hpp"
#include "part.hpp"
#include "unit.hpp"
#include "entity.hpp"
#include "symbol.hpp"
#include "package.hpp"
#include "padstack.hpp"

#include "pool.hpp"
#include "util/editor_process.hpp"

namespace horizon {

	class PoolManagerProcess: public sigc::trackable {
		public:
			enum class Type {IMP_SYMBOL, IMP_PADSTACK, IMP_PACKAGE, UNIT, ENTITY, PART};
			PoolManagerProcess(Type ty, const std::vector<std::string>& args, const std::vector<std::string>& env, class Pool *pool);
			Type type;
			std::unique_ptr<EditorProcess> proc=nullptr;
			Gtk::Window *win = nullptr;
			typedef sigc::signal<void, int> type_signal_exited;
			type_signal_exited signal_exited() {return s_signal_exited;}
			void reload();
		private:
			type_signal_exited s_signal_exited;
	};

	class PoolNotebook: public Gtk::Notebook {
		public:
			PoolNotebook(const std::string &bp);
			void populate();
			void spawn(PoolManagerProcess::Type type, const std::vector<std::string> &args);
			bool can_close();
			void pool_update();

		private:
			const std::string base_path;
			Pool pool;
			std::map<std::string, PoolManagerProcess> processes;
			std::set<class PoolBrowser*> browsers;
			class PartWizard *part_wizard = nullptr;
	};
}
