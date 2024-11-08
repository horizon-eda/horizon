#pragma once
#include <gtkmm.h>
#include <memory>
#include "util/uuid.hpp"
#include <zmq.hpp>
#include "util/win32_undef.hpp"
#include "nlohmann/json_fwd.hpp"
#include "util/editor_process.hpp"
#include "util/window_state_store.hpp"
#include "pool-prj-mgr-process.hpp"
#include "prj-mgr/prj-mgr_views.hpp"
#include "pool-mgr/view_create_pool.hpp"
#include "common/common.hpp"
#include <thread>

namespace horizon {
using json = nlohmann::json;

enum class PoolUpdateStatus;
enum class CheckSchemaUpdateResult;

class PoolProjectManagerAppWindow : public Gtk::ApplicationWindow {
    friend class PoolProjectManagerViewProject;
    friend class PoolProjectManagerViewCreateProject;

public:
    PoolProjectManagerAppWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &refBuilder,
                                class PoolProjectManagerApplication &app);
    ~PoolProjectManagerAppWindow();

    static PoolProjectManagerAppWindow *create(class PoolProjectManagerApplication &app);

    void open_file_view(const Glib::RefPtr<Gio::File> &file);
    void prepare_close();
    bool close_pool_or_project();
    bool really_close_pool_or_project();
    void wait_for_all_processes();
    std::string get_filename() const;

    enum class SpawnFlags { NONE = 0, READ_ONLY = (1 << 0), TEMP = (1 << 1) };

    struct SpawnResult {
        PoolProjectManagerProcess *proc;
        bool spawned; // if false, an existing window got raised
    };

    SpawnResult spawn(PoolProjectManagerProcess::Type type, const std::vector<std::string> &args,
                      SpawnFlags flags = SpawnFlags::NONE);

    std::map<UUID, PoolProjectManagerProcess *> get_processes();

    class Pool *pool = nullptr;
    class PoolParametric *pool_parametric = nullptr;

    typedef sigc::signal<void, std::string, int, bool> type_signal_process_exited;
    type_signal_process_exited signal_process_exited()
    {
        return s_signal_process_exited;
    }

    typedef sigc::signal<void, std::string> type_signal_process_saved;
    type_signal_process_saved signal_process_saved()
    {
        return s_signal_process_saved;
    }

    class ClosePolicy {
    public:
        bool can_close = true;
        std::string reason;
        std::vector<UUID> procs_need_save;
    };

    ClosePolicy get_close_policy();

    std::string get_proc_filename(const UUID &uu);
    void process_save(const UUID &uu);
    void process_close(const UUID &uu);
    bool cleanup_pool_cache(Gtk::Window *parent);

    enum class ViewMode { OPEN, POOL, PROJECT, CREATE_PROJECT, CREATE_POOL };
    ViewMode get_view_mode() const;

    UUID get_pool_uuid() const;
    void pool_notebook_go_to(ObjectType type, const UUID &uu);
    void pool_notebook_show_settings_tab();
    void open_pool(const std::string &pool_json);
    void update_pool_cache_status_now();
    const std::string &get_project_title() const;

    void pool_update(const std::vector<std::string> &filenames = {});

    PoolProjectManagerApplication &app;

private:
    Glib::RefPtr<Gtk::Builder> builder;
    Gtk::Stack *stack = nullptr;
    Gtk::Button *button_open = nullptr;
    Gtk::Button *button_close = nullptr;
    Gtk::Button *button_update = nullptr;
    Gtk::Button *button_cancel = nullptr;
    Gtk::MenuButton *button_new = nullptr;
    Gtk::Button *button_create = nullptr;
    Gtk::Button *button_save = nullptr;
    Gtk::MenuButton *hamburger_menu_button = nullptr;

    Gtk::HeaderBar *header = nullptr;
    Gtk::ListBox *recent_pools_listbox = nullptr;
    Gtk::ListBox *recent_projects_listbox = nullptr;
    Gtk::SearchEntry *recent_pools_search_entry = nullptr;
    Gtk::SearchEntry *recent_projects_search_entry = nullptr;
    Gtk::Stack *recent_pools_placeholder_stack = nullptr;
    Gtk::Stack *recent_projects_placeholder_stack = nullptr;
    std::vector<Gtk::ListBox *> recent_listboxes;
    Gtk::Box *pool_box = nullptr;
    class PoolNotebook *pool_notebook = nullptr;

    Gtk::Label *pool_update_status_label = nullptr;
    Gtk::Revealer *pool_update_status_rev = nullptr;
    Gtk::Button *pool_update_status_close_button = nullptr;
    Gtk::ProgressBar *pool_update_progress = nullptr;
    sigc::connection pool_update_conn;

    Gtk::InfoBar *info_bar = nullptr;
    Gtk::Label *info_bar_label = nullptr;
    Gtk::Button *show_output_button = nullptr;

    Gtk::InfoBar *info_bar_pool_not_added = nullptr;

    Gtk::MenuItem *menu_new_pool = nullptr;
    Gtk::MenuItem *menu_new_project = nullptr;

    class OutputWindow *output_window = nullptr;

    Gtk::InfoBar *info_bar_pool_doc = nullptr;

    Gtk::InfoBar *info_bar_version = nullptr;
    Gtk::Label *version_label = nullptr;

    Gtk::InfoBar *info_bar_gitignore = nullptr;

    Gtk::InfoBar *info_bar_installation_uuid_mismatch = nullptr;

    std::unique_ptr<class Project> project;
    std::string project_filename;
    bool project_needs_save = false;
    void save_project();
    class PartBrowserWindow *part_browser_window = nullptr;
    void cleanup();
#if GTK_CHECK_VERSION(3, 24, 0)
    static gboolean part_browser_key_pressed(GtkEventControllerKey *controller, guint keyval, guint keycode,
                                             GdkModifierType state, gpointer user_data);
#endif

    ViewMode view_mode = ViewMode::OPEN;
    void set_view_mode(ViewMode mode);

    void update_recent_items();
    std::optional<std::string> peek_recent_name_from_path(const std::string &path);
    std::optional<std::string> peek_recent_name(const std::string &path);

    void handle_open();
    void handle_close();
    void handle_recent();
    void handle_update();
    void handle_new_project();
    void handle_new_pool();
    void handle_create();
    void handle_cancel();
    void handle_save();
    json handle_req(const json &j);

    bool on_delete_event(GdkEventAny *ev) override;

    WindowStateStore state_store;

    std::map<UUID, PoolProjectManagerProcess> processes;
    std::map<int, bool> pids_need_save;
    PoolProjectManagerProcess *find_process(const std::string &filename);
    PoolProjectManagerProcess *find_top_schematic_process();
    PoolProjectManagerProcess *find_board_process();

    const UUID uuid_pool_manager = "1b9eecbe-7408-4b99-9aec-170d3658004a";
    const UUID uuid_project_manager = "144a4ad6-4c7c-4136-9920-f58f954c678e";

    type_signal_process_exited s_signal_process_exited;
    type_signal_process_saved s_signal_process_saved;

    PoolProjectManagerViewCreateProject view_create_project;
    PoolProjectManagerViewProject view_project;
    PoolProjectManagerViewCreatePool view_create_pool;

    void handle_place_part(const UUID &uu);
    void handle_assign_part(const UUID &uu);

    zmq::socket_t sock_mgr;
    std::string sock_mgr_ep;

    bool check_pools();
    bool check_schema_update(const std::string &base_path);

    void check_pool_update(const std::string &base_path);
    void check_mtimes(const std::string &base_path);
    std::thread check_mtimes_thread;
    Glib::Dispatcher check_mtimes_dispatcher;

    bool check_autosave(PoolProjectManagerProcess::Type type, const std::vector<std::string> &filenames);

    void set_version_info(const std::string &s);
    bool project_read_only = false;

    bool migrate_project(const std::string &path);
    std::string project_title;

    Glib::Dispatcher pool_update_dispatcher;
    bool in_pool_update_handler = false;
    std::mutex pool_update_status_queue_mutex;
    std::list<std::tuple<PoolUpdateStatus, std::string, std::string>> pool_update_status_queue;
    std::list<std::tuple<PoolUpdateStatus, std::string, std::string>> pool_update_error_queue;
    bool pool_updating = false;
    void pool_updated(bool success);
    std::string pool_update_last_file;
    std::string pool_update_last_info;
    unsigned int pool_update_n_files = 0;
    unsigned int pool_update_n_files_last = 0;
    std::vector<std::string> pool_update_filenames;

    void pool_update_thread();
    void handle_pool_update_progress();
    void set_pool_updating(bool v, bool success);
    void set_pool_update_status_text(const std::string &txt);
    void set_pool_update_progress(float progress);

    std::string get_pool_base_path() const;

    std::string get_gitignore_filename() const;

    void update_recent_search(Gtk::SearchEntry &entry, Gtk::ListBox &lb);
    void update_recent_searches();
    void clear_recent_searches();

    void update_recent_placeholder(Gtk::Stack &stack, Gtk::ListBox &lb);

public:
    zmq::context_t &zctx;
};
}; // namespace horizon
