#pragma once
#include <gtkmm.h>
#include "common/common.hpp"
#include <memory>
#include "util/window_state_store.hpp"
#include "util/pool_goto_provider.hpp"
#include "util/item_set.hpp"
#include "rules/rules.hpp"

namespace horizon {

class EditorWindow : public Gtk::Window, public PoolGotoProvider {
public:
    EditorWindow(ObjectType type, const std::string &filename, class IPool *p, class PoolParametric *pp, bool read_only,
                 bool is_temp);
    void reload();
    bool get_need_update() const;
    void save();
    void force_close();
    bool get_needs_save() const;
    std::string get_filename() const;
    void set_original_filename(const std::string &s);
    ObjectType get_object_type() const;
    const UUID &get_uuid() const;

    void select(const ItemSet &items);

    typedef sigc::signal<void, std::string> type_signal_filename_changed;
    type_signal_filename_changed signal_filename_changed()
    {
        return s_signal_filename_changed;
    }
    type_signal_filename_changed signal_saved()
    {
        return s_signal_saved;
    }

    type_signal_goto signal_open_item()
    {
        return s_signal_open_item;
    }


private:
    ObjectType type;
    class PoolEditorBase *iface = nullptr;
    Gtk::Button *save_button = nullptr;
    Gtk::MenuButton *check_button = nullptr;
    Gtk::Stack *check_button_stack = nullptr;
    class ColorBox *check_color_box = nullptr;
    Gtk::Popover *check_popover = nullptr;
    Gtk::Label *check_label = nullptr;
    Gtk::InfoBar *info_bar = nullptr;
    Gtk::Label *info_bar_label = nullptr;
    class IPool &pool;
    class PoolParametric *pool_parametric;
    bool need_update = false;
    std::string original_filename;

    type_signal_filename_changed s_signal_filename_changed;
    type_signal_filename_changed s_signal_saved;

    WindowStateStore state_store;
    void run_checks();
    bool read_only;

    unsigned int saved_version = 0;
    void update_version_warning();

    type_signal_goto s_signal_open_item;
};
} // namespace horizon
