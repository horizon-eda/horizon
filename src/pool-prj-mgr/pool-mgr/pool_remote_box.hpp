#pragma once
#include <gtkmm.h>
#include <set>
#include <mutex>
#include "util/uuid.hpp"
#include "common/common.hpp"
#include "nlohmann/json.hpp"
#include <git2/sys/repository.h>
#include "util/status_dispatcher.hpp"

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


    void handle_remote_upgrade();
    void handle_create_pr();
    void update_body_placeholder_label();


    void remote_upgrade_thread();
    void create_pr_thread();
    void refresh_prs_thread();
    void checkout_master(git_repository *repo);

    Glib::Dispatcher git_thread_dispatcher;

    enum class GitThreadMode { UPGRADE, PULL_REQUEST };
    GitThreadMode git_thread_mode = GitThreadMode::UPGRADE;
    bool git_thread_busy = false;
    std::string git_thread_status;
    bool git_thread_error = false;
    json pull_requests;
    std::mutex git_thread_mutex;

    std::string gh_owner;
    std::string gh_repo;

    std::set<std::pair<ObjectType, UUID>> items_merge;
    std::set<std::string> models_merge;
    void update_items_merge();
    std::set<std::pair<ObjectType, UUID>> get_referenced(ObjectType ty, const UUID &uu);
    bool exists_in_pool(class Pool &pool, ObjectType ty, const UUID &uu);

    void update_prs();

    std::string gh_username;
    std::string gh_password;

    std::string pr_title;
    std::string pr_body;
};
} // namespace horizon
