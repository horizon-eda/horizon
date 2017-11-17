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
#include <zmq.hpp>

namespace horizon {

	class PoolManagerProcess: public sigc::trackable {
		public:
			enum class Type {IMP_SYMBOL, IMP_PADSTACK, IMP_PACKAGE, UNIT, ENTITY, PART};
			PoolManagerProcess(Type ty, const std::vector<std::string>& args, const std::vector<std::string>& env, class Pool *pool);
			Type type;
			std::unique_ptr<EditorProcess> proc=nullptr;
			class EditorWindow *win = nullptr;
			typedef sigc::signal<void, int, bool> type_signal_exited;
			type_signal_exited signal_exited() {return s_signal_exited;}
			void reload();
		private:
			type_signal_exited s_signal_exited;
	};

	class PoolNotebook: public Gtk::Notebook {
		public:
			PoolNotebook(const std::string &bp, class PoolManagerAppWindow *aw);
			void populate();
			void spawn(PoolManagerProcess::Type type, const std::vector<std::string> &args);
			bool can_close();
			void pool_update();
			~PoolNotebook();

		private:
			const std::string base_path;
			Pool pool;
			class PoolManagerAppWindow *appwin;
			std::map<std::string, PoolManagerProcess> processes;
			std::set<class PoolBrowser*> browsers;
			class PartWizard *part_wizard = nullptr;

			zmq::context_t &zctx;
			zmq::socket_t sock_pool_update;
			std::string sock_pool_update_ep;
			sigc::connection sock_pool_update_conn;
			bool pool_updating = false;
			void pool_updated(bool success);
			std::string pool_update_last_file;
	};
}
