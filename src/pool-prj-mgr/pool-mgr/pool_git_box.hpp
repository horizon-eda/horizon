#pragma once
#include <gtkmm.h>
#include <set>
#include <mutex>
#include "util/uuid.hpp"
#include "common/common.hpp"
#include "nlohmann/json.hpp"
#include "util/sqlite.hpp"
#include <git2.h>
#include "util/sort_controller.hpp"
#include "rules/rules.hpp"

class git_repository;


namespace horizon {
using json = nlohmann::json;

class PoolGitBox : public Gtk::Box {
public:
    PoolGitBox(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, class PoolNotebook *nb);
    static PoolGitBox *create(class PoolNotebook *nb);

    void refresh();
    bool refreshed_once = false;

private:
    class PoolNotebook *notebook = nullptr;

    Gtk::Button *refresh_button = nullptr;
    Gtk::Label *info_label = nullptr;
    Gtk::TreeView *diff_treeview = nullptr;
    Gtk::TreeView *status_treeview = nullptr;
    Gtk::CheckButton *diff_show_deleted_checkbutton = nullptr;
    Gtk::CheckButton *diff_show_modified_checkbutton = nullptr;
    Gtk::Box *diff_box = nullptr;
    Gtk::Button *add_with_deps_button = nullptr;

    Gtk::Button *pr_button = nullptr;
    Gtk::Button *back_to_master_button = nullptr;
    Gtk::Button *back_to_master_delete_button = nullptr;

    void make_treeview(Gtk::TreeView *treeview);

    class TreeColumns : public Gtk::TreeModelColumnRecord {
    public:
        TreeColumns()
        {
            Gtk::TreeModelColumnRecord::add(name);
            Gtk::TreeModelColumnRecord::add(type);
            Gtk::TreeModelColumnRecord::add(uuid);

            Gtk::TreeModelColumnRecord::add(status);
            Gtk::TreeModelColumnRecord::add(path);
            Gtk::TreeModelColumnRecord::add(check_result);
        }
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<ObjectType> type;
        Gtk::TreeModelColumn<UUID> uuid;

        Gtk::TreeModelColumn<unsigned int> status;
        Gtk::TreeModelColumn<std::string> path;
        Gtk::TreeModelColumn<RulesCheckResult> check_result;
    };
    TreeColumns list_columns;

    Glib::RefPtr<Gtk::ListStore> diff_store;
    Glib::RefPtr<Gtk::ListStore> status_store;

    std::optional<SQLite::Query> q_diff;
    std::optional<SQLite::Query> q_status;

    std::optional<SortController> sort_controller_diff;
    std::optional<SortController> sort_controller_status;

    void update_diff();
    void update_status();

    enum class View { DIFF, STATUS };

    void store_from_db(View view, const std::string &extra_q = "");

    static int diff_file_cb_c(const git_diff_delta *delta, float progress, void *pl);
    static int status_cb_c(const char *path, unsigned int status_flags, void *payload);
    void status_cb(const char *path, unsigned int status_flags);
    void diff_file_cb(const git_diff_delta *delta);

    void handle_add_with_deps();
    void handle_pr();
    void handle_back_to_master(bool delete_pr);
};
} // namespace horizon
