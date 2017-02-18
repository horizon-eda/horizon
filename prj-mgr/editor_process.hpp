#pragma once
#include <glibmm.h>

namespace horizon {
	class EditorProcess: public sigc::trackable {
		public:
			EditorProcess(const std::vector<std::string> &argv, const std::vector<std::string> &env);
			int get_pid() const;
			typedef sigc::signal<void, int> type_signal_exited;
			type_signal_exited signal_exited() {return s_signal_exited;}

		private:
			Glib::Pid pid = 0;
			bool is_running = false;
			void child_watch_handler(GPid pid, int child_status);
			type_signal_exited s_signal_exited;
	};
}
