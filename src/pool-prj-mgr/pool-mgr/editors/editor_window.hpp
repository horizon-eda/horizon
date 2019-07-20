#pragma once
#include <gtkmm.h>
#include "common/common.hpp"
#include <memory>
#include "editor_interface.hpp"
#include "util/window_state_store.hpp"
#include "util/pool_goto_provider.hpp"

namespace horizon {
class EditorWindowStore {
public:
    EditorWindowStore(const std::string &fn);
    void save();
    virtual void save_as(const std::string &fn) = 0;
    virtual std::string get_name() const = 0;
    std::string filename;
    virtual ~EditorWindowStore()
    {
    }
};

class EditorWindow : public Gtk::Window, public PoolGotoProvider {
public:
    EditorWindow(ObjectType type, const std::string &filename, class Pool *p, class PoolParametric *pp, bool read_only,
                 bool is_temp);
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
    class PoolParametric *pool_parametric;
    bool need_update = false;

    WindowStateStore state_store;
};
} // namespace horizon
