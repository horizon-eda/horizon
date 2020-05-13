#pragma once
#include <gtkmm.h>

namespace horizon {
class PoolPreferencesEditor : public Gtk::ScrolledWindow {
public:
    PoolPreferencesEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x);
    static PoolPreferencesEditor *create();
    void add_pool(const std::string &path);


private:
    class PoolManager &mgr;
    Gtk::ListBox *listbox = nullptr;
    Glib::RefPtr<Gtk::SizeGroup> size_group;
    void update();
};

} // namespace horizon
