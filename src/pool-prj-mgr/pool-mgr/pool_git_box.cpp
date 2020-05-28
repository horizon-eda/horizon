#include "pool_git_box.hpp"
#include <git2.h>
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
        tvc->set_sort_column(list_columns.type);
        treeview->append_column(*tvc);
    }
    {
        auto tvc = tree_view_append_column_ellipsis(treeview, "Name", list_columns.name, Pango::ELLIPSIZE_END);
        tvc->set_min_width(200);
        tvc->set_resizable(true);
    }
    tree_view_append_column_ellipsis(treeview, "Path", list_columns.path, Pango::ELLIPSIZE_START);

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

void PoolGitBox::install_sort(Glib::RefPtr<Gtk::TreeSortable> store)
{
    store->set_sort_func(list_columns.type,
                         [this](const Gtk::TreeModel::iterator &a, const Gtk::TreeModel::iterator &b) {
                             Gtk::TreeModel::Row ra = *a;
                             Gtk::TreeModel::Row rb = *b;
                             ObjectType ta = ra[list_columns.type];
                             ObjectType tb = rb[list_columns.type];
                             return static_cast<int>(ta) - static_cast<int>(tb);
                         });
    store->set_sort_func(list_columns.status,
                         [this](const Gtk::TreeModel::iterator &a, const Gtk::TreeModel::iterator &b) {
                             Gtk::TreeModel::Row ra = *a;
                             Gtk::TreeModel::Row rb = *b;
                             git_delta_t ta = ra[list_columns.status];
                             git_delta_t tb = rb[list_columns.status];
                             return static_cast<int>(ta) - static_cast<int>(tb);
                         });
    store->set_sort_column(list_columns.status_flags, Gtk::SORT_ASCENDING);
}

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
    x->get_widget("git_add_with_deps_button", add_with_deps_button);
    refresh_button->signal_clicked().connect(sigc::mem_fun(this, &PoolGitBox::refresh));
    add_with_deps_button->signal_clicked().connect(sigc::mem_fun(this, &PoolGitBox::handle_add_with_deps));
    status_treeview->get_selection()->signal_changed().connect(
            [this] { add_with_deps_button->set_sensitive(status_treeview->get_selection()->count_selected_rows()); });

    diff_show_deleted_checkbutton->signal_toggled().connect([this] { diff_store_filtered->refilter(); });
    diff_show_modified_checkbutton->signal_toggled().connect([this] { diff_store_filtered->refilter(); });

    diff_store = Gtk::ListStore::create(list_columns);
    status_store = Gtk::ListStore::create(list_columns);
    status_store_sorted = Gtk::TreeModelSort::create(status_store);

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
    diff_store_sorted = Gtk::TreeModelSort::create(diff_store_filtered);

    install_sort(status_store_sorted);
    install_sort(diff_store_sorted);

    diff_treeview->set_model(diff_store_sorted);
    status_treeview->set_model(status_store_sorted);
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
        tvc->set_sort_column(list_columns.status);
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
        tvc->set_sort_column(list_columns.status_flags);
        status_treeview->append_column(*tvc);
    }
    make_treeview(status_treeview);
}

void PoolGitBox::diff_file_cb(const git_diff_delta *delta)
{
    Gtk::TreeModel::Row row = *diff_store->append();
    row[list_columns.status] = delta->status;
    row[list_columns.path] = delta->new_file.path;
    row[list_columns.type] = ObjectType::INVALID;
    SQLite::Query q(notebook->pool.db, "INSERT INTO 'git_files' VALUES (?)");
    q.bind(1, std::string(delta->new_file.path));
    q.step();
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
        SQLite::Query q(notebook->pool.db, "INSERT INTO 'git_files' VALUES (?)");
        q.bind(1, std::string(path));
        q.step();
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
            diff_treeview->unset_model();
            diff_store->clear();
            update_store_from_db_prepare();
            git_diff_foreach(diff, &PoolGitBox::diff_file_cb_c, nullptr, nullptr, nullptr, this);
            update_store_from_db(diff_store);
            diff_treeview->set_model(diff_store_sorted);
            diff_box->set_visible(tree_master != tree_head);
        }

        {
            status_treeview->unset_model();
            status_store->clear();
            update_store_from_db_prepare();
            git_status_foreach(repo, &PoolGitBox::status_cb_c, this);
            update_store_from_db(status_store);
            status_treeview->set_model(status_store_sorted);
        }
    }
    catch (const std::exception &e) {
        info_label->set_text("exception: " + std::string(e.what()));
    }
    catch (...) {
        info_label->set_text("unknown exception");
    }
}

void PoolGitBox::update_store_from_db_prepare()
{
    notebook->pool.db.execute("CREATE TEMP TABLE 'git_files' ('git_filename' TEXT NOT NULL);");
    notebook->pool.db.execute("BEGIN TRANSACTION");
}

void PoolGitBox::update_store_from_db(Glib::RefPtr<Gtk::ListStore> store)
{
    notebook->pool.db.execute("COMMIT");
    notebook->pool.db.execute("CREATE INDEX git_file ON git_files (git_filename)");
    SQLite::Query q(
            notebook->pool.db,
            "SELECT type, uuid, name, filename FROM git_files LEFT JOIN "
            "(SELECT type, uuid, name, filename FROM all_items_view UNION ALL SELECT DISTINCT 'model_3d' AS type, "
            "'00000000-0000-0000-0000-000000000000' as uuid, '' as name, model_filename as filename FROM models) "
            "ON filename=git_filename");
    auto it = store->children().begin();
    while (q.step()) {
        if (it != store->children().end()) {
            if (it->get_value(list_columns.path) == q.get<std::string>(3)) {
                ObjectType type = object_type_lut.lookup(q.get<std::string>(0));
                UUID uu = q.get<std::string>(1);
                std::string name = q.get<std::string>(2);
                it->set_value(list_columns.type, type);
                it->set_value(list_columns.uuid, uu);
                it->set_value(list_columns.name, Glib::ustring(name));
            }
            else {
                it->set_value(list_columns.name, Glib::ustring("Not found in pool"));
            }
            it++;
        }
        else {
            throw std::runtime_error("db vs store mismatch");
        }
    }

    notebook->pool.db.execute("DROP TABLE 'git_files'");
}

void PoolGitBox::handle_add_with_deps()
{
    auto it = status_treeview->get_selection()->get_selected();
    if (it) {
        Gtk::TreeModel::Row row = *it;
        UUID uu = row[list_columns.uuid];
        ObjectType type = row[list_columns.type];
        SQLite::Query q(notebook->pool.db,
                        "WITH RECURSIVE deps(typex, uuidx) AS "
                        "( SELECT ?, ? UNION "
                        "SELECT dep_type, dep_uuid FROM dependencies, deps "
                        "WHERE dependencies.type = deps.typex AND dependencies.uuid = deps.uuidx) "
                        ", deps_sym(typey, uuidy) AS (SELECT * FROM deps UNION SELECT 'symbol', symbols.uuid FROM "
                        "symbols INNER JOIN deps ON (symbols.unit = uuidx AND typex = 'unit')) "
                        "SELECT filename FROM deps_sym INNER JOIN all_items_view "
                        "ON(all_items_view.type =deps_sym.typey AND all_items_view.uuid = deps_sym.uuidy) "
                        "UNION SELECT model_filename FROM models INNER JOIN deps "
                        "ON (deps.typex = 'package' AND deps.uuidx = models.package_uuid)");
        q.bind(1, object_type_lut.lookup_reverse(type));
        q.bind(2, uu);

        autofree_ptr<git_repository> repo(git_repository_free);
        if (git_repository_open(&repo.ptr, notebook->base_path.c_str()) != 0) {
            throw std::runtime_error("error opening repo");
        }

        autofree_ptr<git_index> index(git_index_free);
        if (git_repository_index(&index.ptr, repo) != 0) {
            throw std::runtime_error("index error");
        }

        while (q.step()) {
            std::string filename = q.get<std::string>(0);
            if (git_index_add_bypath(index, filename.c_str()) != 0) {
                auto last_error = giterr_last();
                std::string err_str = last_error->message;
                throw std::runtime_error("add error: " + err_str);
            }
        }

        if (git_index_write(index) != 0) {
            throw std::runtime_error("error saving index");
        }

        refresh();
    }
}

PoolGitBox *PoolGitBox::create(PoolNotebook *nb)
{
    PoolGitBox *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    std::vector<Glib::ustring> widgets = {"box_git", "sg_git_header"};
    x->add_from_resource("/org/horizon-eda/horizon/pool-prj-mgr/window.ui", widgets);
    x->get_widget_derived("box_git", w, nb);
    w->reference();
    return w;
}

} // namespace horizon
