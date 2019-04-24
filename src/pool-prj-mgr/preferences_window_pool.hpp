#pragma once
#include <gtkmm.h>

namespace horizon {
class PoolPreferencesEditor : public Gtk::Box {
public:
    PoolPreferencesEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x);
    static PoolPreferencesEditor *create();
    void add_pool();


private:
    class PoolManager &mgr;
    Gtk::ListBox *listbox = nullptr;
    Gtk::Button *button_add_pool = nullptr;
    Glib::RefPtr<Gtk::SizeGroup> size_group;
    void update();
};

} // namespace horizon
