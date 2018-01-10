#include "editor_process.hpp"
#include <assert.h>
#include <iostream>
#ifdef G_OS_WIN32
	#include <windows.h>
#endif

namespace horizon {
	EditorProcess::EditorProcess(const std::vector<std::string> &argv, const std::vector<std::string> &env) {
		std::cout << "spawn ";
		for(const auto &it: argv) {
			std::cout << it << " ";
		}
		std::cout << std::endl;
		Glib::spawn_async("" /*cwd*/, argv, env, Glib::SPAWN_DO_NOT_REAP_CHILD|Glib::SPAWN_SEARCH_PATH, Glib::SlotSpawnChildSetup(), &pid);
		is_running = true;
		Glib::signal_child_watch().connect(sigc::mem_fun(this, &EditorProcess::child_watch_handler), pid);
	}

	void EditorProcess::child_watch_handler(Glib::Pid p, int child_status) {
		assert(p==pid);
		std::cout << "end proc " << pid <<std::endl;
		Glib::spawn_close_pid(pid);
		is_running = false;
		s_signal_exited.emit(child_status);
	}

	int EditorProcess::get_pid() const {
		if(!is_running)
			return 0;
		#ifdef G_OS_WIN32
			return GetProcessId(pid);
		#else
			return pid;
		#endif
	}
}
