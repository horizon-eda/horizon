#pragma once
#include <gtkmm.h>
#include <memory>
#include "util/uuid.hpp"
#include <zmq.hpp>
#ifdef G_OS_WIN32
#undef ERROR
#undef DELETE
#undef DUPLICATE
#endif
#include "nlohmann/json_fwd.hpp"
#include "util/editor_process.hpp"
#include "util/window_state_store.hpp"
#include "util/status_dispatcher.hpp"
#include "pool-prj-mgr-process.hpp"
#include "project/project.hpp"
#include "prj-mgr/prj-mgr_views.hpp"
#include "pool-mgr/view_create_pool.hpp"

namespace horizon {
using json = nlohmann::json;

class PoolProjectManagerAppWindow : public Gtk::ApplicationWindow {
    friend class PoolProjectManagerViewProject;
    friend class PoolProjectManagerViewCreateProject;

public:
    PoolProjectManagerAppWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &refBuilder,
                                class PoolProjectManagerApplication *app);
    ~PoolProjectManagerAppWindow();

    static PoolProjectManagerAppWindow *create(class PoolProjectManagerApplication *app);

    void open_file_view(const Glib::RefPtr<Gio::File> &file);
    void prepare_close();
    bool close_pool_or_project();
    bool really_close_pool_or_project();
    void wait_for_all_processes();
    std::string get_filename() const;

    PoolProjectManagerProcess *spawn(PoolProjectManagerProcess::Type type, const std::vector<std::string> &args,
                                     const std::vector<std::string> &env = {}, bool read_only = false);
    PoolProjectManagerProcess *spawn_for_project(PoolProjectManagerProcess::Type type,
                                                 const std::vector<std::string> &args);

    std::map<std::string, PoolProjectManagerProcess *> get_processes();

    class Pool *pool = nullptr;
    class PoolParametric *pool_parametric = nullptr;

    typedef sigc::signal<void, std::string, int, bool> type_signal_process_exited;
    type_signal_process_exited signal_process_exited()
    {
        return s_signal_process_exited;
    }
    void reload();

    class ClosePolicy {
    public:
        bool can_close = true;
        std::string reason;
        std::vector<std::string> files_need_save;
    };

    ClosePolicy get_close_policy();

    void process_save(const std::string &file);
    void process_close(const std::string &file);
    void cleanup_pool_cache();

private:
    Glib::RefPtr<Gtk::Builder> builder;
    Gtk::Stack *stack = nullptr;
    Gtk::Button *button_open = nullptr;
    Gtk::Button *button_close = nullptr;
    Gtk::Button *button_update = nullptr;
    Gtk::Button *button_download = nullptr;
    Gtk::Button *button_do_download = nullptr;
    Gtk::Button *button_cancel = nullptr;
    Gtk::MenuButton *button_new = nullptr;
    Gtk::Button *button_create = nullptr;
    Gtk::Button *button_save = nullptr;
    Gtk::Spinner *spinner_update = nullptr;
    Gtk::Revealer *download_revealer = nullptr;
    Gtk::Label *download_label = nullptr;
    Gtk::Spinner *download_spinner = nullptr;

    Gtk::Entry *download_gh_username_entry = nullptr;
    Gtk::Entry *download_gh_repo_entry = nullptr;
    Gtk::FileChooserButton *download_dest_dir_button = nullptr;

    Gtk::HeaderBar *header = nullptr;
    Gtk::ListBox *recent_pools_listbox = nullptr;
    Gtk::ListBox *recent_projects_listbox = nullptr;
    std::vector<Gtk::ListBox *> recent_listboxes;
    Gtk::Label *label_gitversion = nullptr;
    Gtk::Box *pool_box = nullptr;
    class PoolNotebook *pool_notebook = nullptr;

    Gtk::Label *pool_update_status_label = nullptr;
    Gtk::Revealer *pool_update_status_rev = nullptr;
    Gtk::Button *pool_update_status_close_button = nullptr;
    Gtk::ProgressBar *pool_update_progress = nullptr;

    Gtk::InfoBar *info_bar = nullptr;
    Gtk::Label *info_bar_label = nullptr;

    Gtk::MenuItem *menu_new_pool = nullptr;
    Gtk::MenuItem *menu_new_project = nullptr;

    std::unique_ptr<Project> project = nullptr;
    std::string project_filename;
    bool project_needs_save = false;
    void save_project();
    class PartBrowserWindow *part_browser_window = nullptr;
    class PoolCacheWindow *pool_cache_window = nullptr;

    enum class ViewMode { OPEN, POOL, DOWNLOAD, PROJECT, CREATE_PROJECT, CREATE_POOL };
    ViewMode view_mode = ViewMode::OPEN;
    void set_view_mode(ViewMode mode);

    void update_recent_items();

    void handle_open();
    void handle_close();
    void handle_recent();
    void handle_update();
    void handle_download();
    void handle_do_download();
    void handle_new_project();
    void handle_new_pool();
    void handle_create();
    void handle_cancel();
    void handle_save();
    json handle_req(const json &j);

    bool on_delete_event(GdkEventAny *ev) override;

    void download_thread(std::string gh_username, std::string gh_repo, std::string dest_dir);

    StatusDispatcher download_status_dispatcher;

    WindowStateStore state_store;

    std::map<std::string, PoolProjectManagerProcess> processes;
    std::map<int, bool> pids_need_save;

    type_signal_process_exited s_signal_process_exited;

    PoolProjectManagerViewCreateProject view_create_project;
    PoolProjectManagerViewProject view_project;
    PoolProjectManagerViewCreatePool view_create_pool;

    void handle_place_part(const UUID &uu);
    void handle_assign_part(const UUID &uu);

    zmq::socket_t sock_mgr;
    std::string sock_mgr_ep;

    bool check_pools();
    bool check_schema_update(const std::string &base_path);

public:
    zmq::context_t &zctx;
    void set_pool_updating(bool v, bool success);
    void set_pool_update_status_text(const std::string &txt);
    void set_pool_update_progress(float progress);
};
}; // namespace horizon
