#pragma once
#include <gtkmm.h>
#include <memory>
#include "util/uuid.hpp"
#include "pool/part.hpp"
#include "pool/unit.hpp"
#include "pool/entity.hpp"
#include "pool/symbol.hpp"
#include "pool/package.hpp"
#include "pool/padstack.hpp"

#include "pool/pool.hpp"
#include "pool/pool_parametric.hpp"
#include "util/editor_process.hpp"
#include "pool-update/pool-update.hpp"
#include <zmq.hpp>
#ifdef G_OS_WIN32
#undef ERROR
#undef DELETE
#undef DUPLICATE
#endif

namespace horizon {
class PoolNotebook : public Gtk::Notebook {
    friend class PoolRemoteBox;

public:
    PoolNotebook(const std::string &bp, class PoolProjectManagerAppWindow *aw);
    void populate();
    bool get_close_prohibited() const;
    void prepare_close();
    void pool_update(std::function<void()> cb = nullptr);
    bool get_needs_save() const;
    void save();
    void go_to(ObjectType type, const UUID &uu);
    ~PoolNotebook();

private:
    const std::string base_path;
    Pool pool;
    PoolParametric pool_parametric;
    class PoolProjectManagerAppWindow *appwin;
    std::map<ObjectType, class PoolBrowser *> browsers;
    std::map<std::string, class PoolBrowserParametric *> browsers_parametric;
    class PartWizard *part_wizard = nullptr;
    class DuplicateWindow *duplicate_window = nullptr;
    bool closing = false;

    Glib::Dispatcher pool_update_dispatcher;
    bool in_pool_update_handler = false;
    std::mutex pool_update_status_queue_mutex;
    std::deque<std::tuple<PoolUpdateStatus, std::string, std::string>> pool_update_status_queue;
    std::deque<std::tuple<PoolUpdateStatus, std::string, std::string>> pool_update_error_queue;
    bool pool_updating = false;
    void pool_updated(bool success);
    std::string pool_update_last_file;
    unsigned int pool_update_n_files = 0;
    unsigned int pool_update_n_files_last = 0;
    std::function<void()> pool_update_done_cb = nullptr;

    void pool_update_thread();

    void show_duplicate_window(ObjectType ty, const UUID &uu);

    void construct_units();
    void handle_create_unit();
    void handle_edit_unit(const UUID &uu);
    void handle_create_symbol_for_unit(const UUID &uu);
    void handle_create_entity_for_unit(const UUID &uu);
    void handle_duplicate_unit(const UUID &uu);

    void construct_symbols();
    void handle_edit_symbol(const UUID &uu);
    void handle_create_symbol();
    void handle_duplicate_symbol(const UUID &uu);

    void construct_entities();
    void handle_edit_entity(const UUID &uu);
    void handle_create_entity();
    void handle_duplicate_entity(const UUID &uu);

    void construct_padstacks();
    void handle_edit_padstack(const UUID &uu);
    void handle_create_padstack();
    void handle_duplicate_padstack(const UUID &uu);

    void construct_packages();
    void handle_edit_package(const UUID &uu);
    void handle_create_package();
    void handle_create_padstack_for_package(const UUID &uu);
    void handle_duplicate_package(const UUID &uu);
    void handle_part_wizard(const UUID &uu);

    void construct_parts();
    void handle_edit_part(const UUID &uu);
    void handle_create_part();
    void handle_create_part_from_part(const UUID &uu);
    void handle_duplicate_part(const UUID &uu);

    void construct_frames();
    void handle_edit_frame(const UUID &uu);
    void handle_create_frame();
    void handle_duplicate_frame(const UUID &uu);

    Gtk::Button *add_action_button(const std::string &label, Gtk::Box *bbox, sigc::slot0<void>);
    Gtk::Button *add_action_button(const std::string &label, Gtk::Box *bbox, class PoolBrowser *br,
                                   sigc::slot1<void, UUID>);
    void add_preview_stack_switcher(Gtk::Box *bbox, Gtk::Stack *stack);

    void handle_delete(ObjectType ty, const UUID &uu);
    void handle_copy_path(ObjectType ty, const UUID &uu);
    void add_context_menu(class PoolBrowser *br);

    std::string remote_repo;
    class PoolRemoteBox *remote_box = nullptr;
    class PoolSettingsBox *settings_box = nullptr;

    UUID pool_uuid;
};
} // namespace horizon
