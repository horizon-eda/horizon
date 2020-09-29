#pragma once
#include <gtkmm.h>
#include <thread>

namespace horizon {
class GitHubLoginWindow : public Gtk::Window {
public:
    static GitHubLoginWindow *create(const std::string &auth_filename, const std::string &client_id);
    GitHubLoginWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, const std::string &auth_filename,
                      const std::string &client_id);

private:
    Gtk::Stack *stack = nullptr;
    Gtk::Label *code_label = nullptr;
    Gtk::Label *done_label = nullptr;
    Gtk::Label *error_label = nullptr;
    Gtk::Button *copy_button = nullptr;
    Gtk::Button *browser_button = nullptr;

    std::thread thread;
    void worker_thread();
    Glib::Dispatcher dispatcher;

    std::string user_code;
    std::string verification_uri;

    std::string token;
    std::string user;
    std::string error_msg;
    bool cancel = false;

    const std::string auth_filename;
    const std::string client_id;
};
} // namespace horizon
