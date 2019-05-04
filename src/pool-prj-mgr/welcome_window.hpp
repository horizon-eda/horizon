#pragma once
#include <array>
#include <gtkmm.h>
#include <set>

namespace horizon {

class WelcomeWindow : public Gtk::Window {
public:
    WelcomeWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, class PoolProjectManagerAppWindow *aw);
    static WelcomeWindow *create(class PoolProjectManagerAppWindow *aw);


private:
    class PoolProjectManagerAppWindow *appwin;
    Gtk::DrawingArea *banner_area = nullptr;
    Glib::RefPtr<Gdk::Pixbuf> pixbuf;
    Gtk::Button *button_back = nullptr;
    Gtk::Stack *stack = nullptr;
    Gtk::Button *button_git_add_pool = nullptr;

    void handle_open();

    bool draw_banner(const Cairo::RefPtr<Cairo::Context> &cr);
};
} // namespace horizon
