#pragma once
#include <glibmm.h>

namespace horizon {
class EditorProcess {
public:
    EditorProcess(const std::vector<std::string> &argv, const std::vector<std::string> &env, bool capture_output);
    int get_pid() const;
    typedef sigc::signal<void, int> type_signal_exited;
    type_signal_exited signal_exited()
    {
        return s_signal_exited;
    }
    typedef sigc::signal<void, std::string, bool> type_signal_output;
    type_signal_output signal_output()
    {
        return s_signal_output;
    }

private:
    Glib::Pid pid = 0;
    Glib::RefPtr<Glib::IOChannel> ch_stdout;
    Glib::RefPtr<Glib::IOChannel> ch_stderr;
    sigc::connection out_conn;
    sigc::connection err_conn;
    bool is_running = false;
    void child_watch_handler(GPid pid, int child_status);
    type_signal_exited s_signal_exited;
    type_signal_output s_signal_output;
};
} // namespace horizon
