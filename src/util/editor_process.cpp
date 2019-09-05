#include "editor_process.hpp"
#include <assert.h>
#include <iostream>
#ifdef G_OS_WIN32
#include <windows.h>
#endif

namespace horizon {
EditorProcess::EditorProcess(const std::vector<std::string> &argv, const std::vector<std::string> &env,
                             bool capture_output)
{
    std::cout << "spawn ";
    for (const auto &it : argv) {
        std::cout << it << " ";
    }
    std::cout << std::endl;
    int p_stdout = 0;
    int p_stderr = 0;
    if (capture_output) {
        Glib::spawn_async_with_pipes("" /*cwd*/, argv, env, Glib::SPAWN_DO_NOT_REAP_CHILD | Glib::SPAWN_SEARCH_PATH,
                                     Glib::SlotSpawnChildSetup(), &pid, nullptr, &p_stdout, &p_stderr);
#ifdef G_OS_WIN32
        ch_stdout = Glib::IOChannel::create_from_win32_fd(p_stdout);
        ch_stderr = Glib::IOChannel::create_from_win32_fd(p_stderr);
#else
        ch_stdout = Glib::IOChannel::create_from_fd(p_stdout);
        ch_stderr = Glib::IOChannel::create_from_fd(p_stderr);
#endif
        out_conn = Glib::signal_io().connect(sigc::track_obj(
                                                     [this](Glib::IOCondition cond) {
                                                         Glib::ustring line;
                                                         ch_stdout->read_line(line);
                                                         s_signal_output.emit(line, false);
                                                         return true;
                                                     },
                                                     this),
                                             ch_stdout, Glib::IO_IN);
        err_conn = Glib::signal_io().connect(sigc::track_obj(
                                                     [this](Glib::IOCondition cond) {
                                                         Glib::ustring line;
                                                         ch_stderr->read_line(line);
                                                         s_signal_output.emit(line, true);
                                                         return true;
                                                     },
                                                     this),
                                             ch_stderr, Glib::IO_IN);
    }
    else {
        Glib::spawn_async("" /*cwd*/, argv, env, Glib::SPAWN_DO_NOT_REAP_CHILD | Glib::SPAWN_SEARCH_PATH,
                          Glib::SlotSpawnChildSetup(), &pid);
    }


    is_running = true;
    Glib::signal_child_watch().connect(sigc::mem_fun(*this, &EditorProcess::child_watch_handler), pid);
}

void EditorProcess::child_watch_handler(Glib::Pid p, int child_status)
{
    assert(p == pid);
    std::cout << "end proc " << pid << std::endl;
    Glib::spawn_close_pid(pid);
    if (ch_stderr)
        ch_stderr->close();
    if (ch_stdout)
        ch_stdout->close();
    out_conn.disconnect();
    err_conn.disconnect();
    is_running = false;
    s_signal_exited.emit(child_status);
}

int EditorProcess::get_pid() const
{
    if (!is_running)
        return 0;
#ifdef G_OS_WIN32
    return GetProcessId(pid);
#else
    return pid;
#endif
}
} // namespace horizon
