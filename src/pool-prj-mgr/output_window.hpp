#pragma once
#include <array>
#include <gtkmm.h>
#include <set>

namespace horizon {

class OutputWindow : public Gtk::Window {
public:
    OutputWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x);
    static OutputWindow *create();
    void handle_output(const std::string &line, bool err);
    void clear_all();

private:
    Gtk::TextView *view_stdout = nullptr;
    Gtk::TextView *view_stderr = nullptr;
    Gtk::Stack *stack = nullptr;

    Gtk::TextView *get_view();
};
} // namespace horizon
