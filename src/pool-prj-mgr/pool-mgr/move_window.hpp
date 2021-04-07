#pragma once
#include <gtkmm.h>
#include "util/window_state_store.hpp"
#include "common/common.hpp"
#include "util/uuid.hpp"

namespace horizon {
class MoveWindow : public Gtk::Window {
public:
    friend class MoveItemRow;
    MoveWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, class Pool &pool, ObjectType type,
               const UUID &uu);
    static MoveWindow *create(class Pool &pool, ObjectType type, const UUID &uu);
    bool get_moved() const
    {
        return moved;
    }

private:
    void do_move();
    class Pool &pool;
    Gtk::ComboBoxText *pool_combo = nullptr;
    Gtk::ListBox *listbox = nullptr;
    bool moved = false;


    Glib::RefPtr<Gtk::SizeGroup> sg_item;
    Glib::RefPtr<Gtk::SizeGroup> sg_type;
    Glib::RefPtr<Gtk::SizeGroup> sg_src;
    Glib::RefPtr<Gtk::SizeGroup> sg_dest;

    WindowStateStore window_state_store;
};
} // namespace horizon
