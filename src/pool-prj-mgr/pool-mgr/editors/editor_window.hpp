#pragma once
#include <gtkmm.h>
#include "common/common.hpp"
#include <memory>
#include "editor_interface.hpp"
#include "util/window_state_store.hpp"

namespace horizon {
class EditorWindowStore {
public:
    EditorWindowStore(const std::string &fn);
    void save();
    virtual void save_as(const std::string &fn) = 0;
    std::string filename;
    virtual ~EditorWindowStore()
    {
    }
};

class EditorWindow : public Gtk::Window {
public:
    EditorWindow(ObjectType type, const std::string &filename, class Pool *p);
    void reload();
    bool get_need_update();
    static std::string fix_filename(std::string s);
    void save();
    void force_close();
    bool get_needs_save();

private:
    ObjectType type;
    std::unique_ptr<EditorWindowStore> store = nullptr;
    PoolEditorInterface *iface = nullptr;
    Gtk::Button *save_button = nullptr;
    class Pool *pool;
    bool need_update = false;

    WindowStateStore state_store;
};
} // namespace horizon
