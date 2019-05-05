#include "pool_git_box.hpp"
#include <git2.h>
#include <git2/cred_helpers.h>
#include <git2/sys/repository.h>
#include "util/autofree_ptr.hpp"
#include "util/gtk_util.hpp"
#include "pool_notebook.hpp"
#include "util/str_util.hpp"
#include "util/util.hpp"
#include "common/object_descr.hpp"

namespace horizon {

void PoolGitBox::make_treeview(Gtk::TreeView *treeview)
{
    {
        auto cr = Gtk::manage(new Gtk::CellRendererText());
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Type", *cr));
        tvc->set_cell_data_func(*cr, [this](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
            Gtk::TreeModel::Row row = *it;
            auto mcr = dynamic_cast<Gtk::CellRendererText *>(tcr);
            if (row[list_columns.type] == ObjectType::INVALID)
                mcr->property_text() = "";
            else
                mcr->property_text() = object_descriptions.at(row[list_columns.type]).name;
        });
        treeview->append_column(*tvc);
    }

    treeview->append_column("Name", list_columns.name);
    treeview->append_column("Path", list_columns.path);

    treeview->signal_row_activated().connect([this, treeview](const Gtk::TreeModel::Path &path,
                                                              Gtk::TreeViewColumn *column) {
        auto it = treeview->get_model()->get_iter(path);
        if (it) {
            Gtk::TreeModel::Row row = *it;
            if ((row[list_columns.type] != ObjectType::INVALID) && (row[list_columns.type] != ObjectType::MODEL_3D)) {
                notebook->go_to(row[list_columns.type], row[list_columns.uuid]);
            }
        }
    });
}

static const std::map<unsigned int, std::string> status_names = {{GIT_STATUS_CURRENT, "Current"},
                                                                 {GIT_STATUS_INDEX_NEW, "Staged new"},
                                                                 {GIT_STATUS_INDEX_MODIFIED, "Staged modified"},
                                                                 {GIT_STATUS_INDEX_DELETED, "Staged deleted"},
                                                                 {GIT_STATUS_INDEX_RENAMED, "Staged renamed"},
                                                                 {GIT_STATUS_INDEX_TYPECHANGE, "Staged type changed"},
                                                                 {GIT_STATUS_WT_NEW, "New"},
                                                                 {GIT_STATUS_WT_MODIFIED, "Modified"},
                                                                 {GIT_STATUS_WT_DELETED, "Deleted"},
                                                                 {GIT_STATUS_WT_TYPECHANGE, "Type changed"},
                                                                 {GIT_STATUS_WT_RENAMED, "Renamed"},
                                                                 {GIT_STATUS_WT_UNREADABLE, "Unreadable"},
                                                                 {GIT_STATUS_IGNORED, "Ignored"},
                                                                 {GIT_STATUS_CONFLICTED, "Conflicted"}};

PoolGitBox::PoolGitBox(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, PoolNotebook *nb)
    : Gtk::Box(cobject), notebook(nb)
{
    x->get_widget("button_git_refresh", refresh_button);
    x->get_widget("label_git_info", info_label);
    x->get_widget("git_diff_treeview", diff_treeview);
    x->get_widget("git_status_treeview", status_treeview);
    x->get_widget("checkbutton_git_diff_show_deleted", diff_show_deleted_checkbutton);
    x->get_widget("checkbutton_git_diff_show_modified", diff_show_modified_checkbutton);
    x->get_widget("git_diff_box", diff_box);
    refresh_button->signal_clicked().connect(sigc::mem_fun(this, &PoolGitBox::refresh));

    diff_show_deleted_checkbutton->signal_toggled().connect([this] { diff_store_filtered->refilter(); });
    diff_show_modified_checkbutton->signal_toggled().connect([this] { diff_store_filtered->refilter(); });

    diff_store = Gtk::ListStore::create(list_columns);
    status_store = Gtk::ListStore::create(list_columns);
    diff_store_filtered = Gtk::TreeModelFilter::create(diff_store);
    diff_store_filtered->set_visible_func([this](const Gtk::TreeModel::const_iterator &it) -> bool {
        const Gtk::TreeModel::Row row = *it;
        git_delta_t status = row[list_columns.status];
        if (status == GIT_DELTA_DELETED)
            return diff_show_deleted_checkbutton->get_active();
        else if (status == GIT_DELTA_MODIFIED)
            return diff_show_modified_checkbutton->get_active();
        else
            return true;
    });
    diff_treeview->set_model(diff_store_filtered);
    status_treeview->set_model(status_store);
    {
        auto cr = Gtk::manage(new Gtk::CellRendererText());
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("State", *cr));
        tvc->set_cell_data_func(*cr, [this](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
            Gtk::TreeModel::Row row = *it;
            auto mcr = dynamic_cast<Gtk::CellRendererText *>(tcr);
            std::string state;
            switch (row[list_columns.status]) {
            case GIT_DELTA_ADDED:
                state = "Added";
                break;

            case GIT_DELTA_DELETED:
                state = "Deleted";
                break;

            case GIT_DELTA_MODIFIED:
                state = "Modified";
                break;

            default:
                state = "Unknown " + std::to_string(static_cast<int>(row[list_columns.status]));
            }
            mcr->property_text() = state;
        });
        diff_treeview->append_column(*tvc);
    }
    make_treeview(diff_treeview);
    {
        auto cr = Gtk::manage(new Gtk::CellRendererText());
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Status", *cr));
        tvc->set_cell_data_func(*cr, [this](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
            Gtk::TreeModel::Row row = *it;
            auto mcr = dynamic_cast<Gtk::CellRendererText *>(tcr);
            unsigned int status_flags = row[list_columns.status_flags];
            std::string s;
            for (const auto &it_st : status_names) {
                if (status_flags & it_st.first) {
                    s += it_st.second + ", ";
                }
            }
            if (s.size()) {
                s.pop_back();
                s.pop_back();
            }
            mcr->property_text() = s;
        });
        status_treeview->append_column(*tvc);
    }
    make_treeview(status_treeview);
}

void PoolGitBox::row_from_filename(Gtk::TreeModel::Row &row, const std::string &filename)
{
    SQLite::Query q(notebook->pool.db, "SELECT type, uuid, name FROM all_items_view WHERE filename = ?");
    q.bind(1, filename);
    if (q.step()) {
        ObjectType type = object_type_lut.lookup(q.get<std::string>(0));
        UUID uu = q.get<std::string>(1);
        std::string name = q.get<std::string>(2);
        row[list_columns.type] = type;
        row[list_columns.uuid] = uu;
        row[list_columns.name] = name;
    }
    else {
        SQLite::Query q_model(notebook->pool.db, "SELECT package_uuid FROM models WHERE model_filename = ?");
        q_model.bind(1, filename);
        if (q_model.step()) {
            row[list_columns.type] = ObjectType::MODEL_3D;
        }
        else {
            row[list_columns.name] = "Not found in pool";
        }
    }
}

void PoolGitBox::diff_file_cb(const git_diff_delta *delta)
{
    Gtk::TreeModel::Row row = *diff_store->append();
    row[list_columns.status] = delta->status;
    row[list_columns.path] = delta->new_file.path;
    row[list_columns.type] = ObjectType::INVALID;
    row_from_filename(row, delta->new_file.path);
}

int PoolGitBox::diff_file_cb_c(const git_diff_delta *delta, float progress, void *pl)
{
    auto self = reinterpret_cast<PoolGitBox *>(pl);
    self->diff_file_cb(delta);
    return 0;
}

int PoolGitBox::status_cb_c(const char *path, unsigned int status_flags, void *payload)
{
    auto self = reinterpret_cast<PoolGitBox *>(payload);
    self->status_cb(path, status_flags);
    return 0;
}

void PoolGitBox::status_cb(const char *path, unsigned int status_flags)
{
    if (!(status_flags & GIT_STATUS_IGNORED)) {
        Gtk::TreeModel::Row row = *status_store->append();
        row[list_columns.path] = path;
        row[list_columns.type] = ObjectType::INVALID;
        row[list_columns.status_flags] = status_flags;
        row_from_filename(row, path);
    }
}

void PoolGitBox::refresh()
{
    try {
        autofree_ptr<git_repository> repo(git_repository_free);
        if (git_repository_open(&repo.ptr, notebook->base_path.c_str()) != 0) {
            throw std::runtime_error("error opening repo");
        }
        autofree_ptr<git_reference> head(git_reference_free);
        if (git_repository_head(&head.ptr, repo) != 0) {
            throw std::runtime_error("error getting head");
        }
        auto shorthand = git_reference_shorthand(head);
        info_label->set_markup("on <tt>" + Glib::Markup::escape_text(std::string(shorthand)) + "</tt>");

        autofree_ptr<git_object> treeish_master(git_object_free);
        if (git_revparse_single(&treeish_master.ptr, repo, "master") != 0) {
            throw std::runtime_error("error finding master branch");
        }

        autofree_ptr<git_object> otree_master(git_object_free);
        if (git_object_peel(&otree_master.ptr, treeish_master, GIT_OBJ_TREE) != 0) {
            throw std::runtime_error("error peeling master");
        }

        autofree_ptr<git_object> treeish_head(git_object_free);
        if (git_reference_peel(&treeish_head.ptr, head, GIT_OBJ_TREE) != 0) {
            throw std::runtime_error("error peeling head");
        }


        autofree_ptr<git_tree> tree_master(git_tree_free);
        autofree_ptr<git_tree> tree_head(git_tree_free);


        if (git_tree_lookup(&tree_master.ptr, repo, git_object_id(otree_master)) != 0) {
            throw std::runtime_error("error finding master tree");
        }
        if (git_tree_lookup(&tree_head.ptr, repo, git_object_id(treeish_head)) != 0) {
            throw std::runtime_error("error finding head tree");
        }

        {
            autofree_ptr<git_diff> diff(git_diff_free);
            git_diff_tree_to_workdir_with_index(&diff.ptr, repo, tree_master, nullptr);

            diff_store->clear();
            git_diff_foreach(diff, &PoolGitBox::diff_file_cb_c, nullptr, nullptr, nullptr, this);
            diff_box->set_visible(tree_master != tree_head);
        }

        {
            status_store->clear();
            git_status_foreach(repo, &PoolGitBox::status_cb_c, this);
        }
    }
    catch (const std::exception &e) {
        info_label->set_text("exception: " + std::string(e.what()));
    }
    catch (...) {
        info_label->set_text("unknown exception");
    }
}

PoolGitBox *PoolGitBox::create(PoolNotebook *nb)
{
    PoolGitBox *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    std::vector<Glib::ustring> widgets = {"box_git"};
    x->add_from_resource("/net/carrotIndustries/horizon/pool-prj-mgr/window.ui", widgets);
    x->get_widget_derived("box_git", w, nb);
    w->reference();
    return w;
}

} // namespace horizon
