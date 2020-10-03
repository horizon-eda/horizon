#pragma once
#include <gtkmm.h>
#include <set>
#include <mutex>
#include "util/uuid.hpp"
#include "common/common.hpp"
#include "nlohmann/json.hpp"
#include <git2.h>
#include "util/status_dispatcher.hpp"
#include "util/item_set.hpp"
#include <atomic>

namespace horizon {
using json = nlohmann::json;

class PoolRemoteBox : public Gtk::Box {
public:
    PoolRemoteBox(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, class PoolNotebook *nb);
    static PoolRemoteBox *create(class PoolNotebook *nb);

    void merge_item(ObjectType ty, const UUID &uu);
    void merge_3d_model(const std::string &filename);
    void handle_refresh_prs();
    bool prs_refreshed_once = false;

    void login_once();

private:
    class PoolNotebook *notebook = nullptr;

    class ListColumns : public Gtk::TreeModelColumnRecord {
    public:
        ListColumns()
        {
            Gtk::TreeModelColumnRecord::add(name);
            Gtk::TreeModelColumnRecord::add(type);
            Gtk::TreeModelColumnRecord::add(uuid);
            Gtk::TreeModelColumnRecord::add(filename);
        }
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<ObjectType> type;
        Gtk::TreeModelColumn<UUID> uuid;
        Gtk::TreeModelColumn<std::string> filename;
    };
    ListColumns list_columns;

    Glib::RefPtr<Gtk::ListStore> item_store;
    Gtk::TreeView *merge_items_view = nullptr;
    Gtk::Label *merge_items_placeholder_label = nullptr;
    Gtk::Label *pr_body_placeholder_label = nullptr;
    Gtk::Button *upgrade_button = nullptr;
    Gtk::Button *create_pr_button = nullptr;
    Gtk::Button *refresh_prs_button = nullptr;
    Gtk::Revealer *upgrade_revealer = nullptr;
    Gtk::Label *upgrade_label = nullptr;
    Gtk::Spinner *upgrade_spinner = nullptr;
    Gtk::Label *gh_repo_link_label = nullptr;
    Gtk::Label *gh_signed_in_label = nullptr;
    Gtk::Button *merge_items_clear_button = nullptr;
    Gtk::Button *merge_items_remove_button = nullptr;
    Gtk::Entry *pr_title_entry = nullptr;
    Gtk::TextView *pr_body_textview = nullptr;
    Gtk::ListBox *pull_requests_listbox = nullptr;
    Gtk::Spinner *pr_spinner = nullptr;
    StatusDispatcher pr_status_dispatcher;
    Gtk::Button *login_button = nullptr;
    Gtk::Button *logout_button = nullptr;
    Gtk::CheckButton *show_only_my_prs_cb = nullptr;
    Gtk::Button *pr_update_button = nullptr;
    Gtk::Button *pr_update_cancel_button = nullptr;


    void handle_remote_upgrade();
    void handle_create_pr();
    void handle_update_pr();
    void update_body_placeholder_label();


    void remote_upgrade_thread();
    void create_pr_thread();
    void update_pr_thread();
    void refresh_prs_thread();
    void login_thread();
    void checkout_master(git_repository *repo);
    std::string get_token_filename() const;
    bool update_login();
    void set_pr_update_mode(unsigned int pr, const std::string branch_name);

    static int pr_diff_file_cb_c(const git_diff_delta *delta, float progress, void *pl);
    void pr_diff_file_cb(const git_diff_delta *delta);

    Glib::Dispatcher git_thread_dispatcher;

    enum class GitThreadMode { UPGRADE, PULL_REQUEST, PULL_REQUEST_UPDATE, LOGIN };
    GitThreadMode git_thread_mode = GitThreadMode::UPGRADE;
    bool git_thread_busy = false;
    std::string git_thread_status;
    bool git_thread_error = false;
    json pull_requests;
    std::mutex git_thread_mutex;

    std::string gh_owner;
    std::string gh_repo;

    ItemSet items_merge;
    std::set<std::string> models_merge;
    void update_items_merge();
    ItemSet get_referenced(ObjectType ty, const UUID &uu);
    bool exists_in_pool(class Pool &pool, ObjectType ty, const UUID &uu);
    git_oid items_to_tree(git_repository *repo);
    void push_branch(git_remote *remote, const std::string &branch_name);

    void update_prs();
    void update_my_prs();

    std::string gh_username;
    std::string gh_token;

    std::string pr_title;
    std::string pr_body;

    unsigned int pr_update_nr = 0;
    std::string pr_update_branch;

    bool logged_in_once = false;
    std::atomic<bool> login_succeeded = true;
};
} // namespace horizon
