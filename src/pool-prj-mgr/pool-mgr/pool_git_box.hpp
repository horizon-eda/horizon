#pragma once
#include <gtkmm.h>
#include <set>
#include <mutex>
#include "util/uuid.hpp"
#include "common/common.hpp"
#include "nlohmann/json.hpp"
#include <git2.h>

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

    void make_treeview(Gtk::TreeView *treeview);

    class TreeColumns : public Gtk::TreeModelColumnRecord {
    public:
        TreeColumns()
        {
            Gtk::TreeModelColumnRecord::add(name);
            Gtk::TreeModelColumnRecord::add(type);
            Gtk::TreeModelColumnRecord::add(uuid);

            Gtk::TreeModelColumnRecord::add(status);
            Gtk::TreeModelColumnRecord::add(status_flags);
            Gtk::TreeModelColumnRecord::add(path);
        }
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<ObjectType> type;
        Gtk::TreeModelColumn<UUID> uuid;

        Gtk::TreeModelColumn<git_delta_t> status;
        Gtk::TreeModelColumn<unsigned int> status_flags;
        Gtk::TreeModelColumn<std::string> path;
    };
    TreeColumns list_columns;

    Glib::RefPtr<Gtk::ListStore> diff_store;
    Glib::RefPtr<Gtk::TreeModelFilter> diff_store_filtered;
    Glib::RefPtr<Gtk::TreeModelSort> diff_store_sorted;

    Glib::RefPtr<Gtk::ListStore> status_store;
    Glib::RefPtr<Gtk::TreeModelSort> status_store_sorted;
    void install_sort(Glib::RefPtr<Gtk::TreeSortable> store);

    static int diff_file_cb_c(const git_diff_delta *delta, float progress, void *pl);
    static int status_cb_c(const char *path, unsigned int status_flags, void *payload);
    void status_cb(const char *path, unsigned int status_flags);
    void diff_file_cb(const git_diff_delta *delta);

    void update_store_from_db_prepare();
    void update_store_from_db(Glib::RefPtr<Gtk::ListStore> store);
    void handle_add_with_deps();
};
} // namespace horizon
