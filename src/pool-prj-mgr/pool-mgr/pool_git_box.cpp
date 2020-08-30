#include "pool_git_box.hpp"
#include <git2.h>
#include "util/autofree_ptr.hpp"
#include "util/gtk_util.hpp"
#include "pool_notebook.hpp"
#include "util/str_util.hpp"
#include "util/util.hpp"
#include "common/object_descr.hpp"
#include "util/github_client.hpp"
#include "pool-prj-mgr/pool-prj-mgr-app_win.hpp"
#include "preferences/preferences_provider.hpp"
#include "preferences/preferences.hpp"
#include <iomanip>

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


    x->get_widget("button_git_pr", pr_button);
    x->get_widget("button_git_back_to_master", back_to_master_button);
    x->get_widget("button_git_back_to_master_delete", back_to_master_delete_button);
    pr_button->signal_clicked().connect(sigc::mem_fun(*this, &PoolGitBox::handle_pr));
    back_to_master_button->signal_clicked().connect([this] { handle_back_to_master(false); });
    back_to_master_delete_button->signal_clicked().connect([this] { handle_back_to_master(true); });

    if (PreferencesProvider::get_prefs().show_pull_request_tools == false) {
        pr_button->hide();
        back_to_master_button->hide();
        back_to_master_delete_button->hide();
    }
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
        const char *branch_name = "fixme";
        if (git_branch_name(&branch_name, head) != 0) {
            throw std::runtime_error("error getting branch name");
        }
        info_label->set_markup("on <tt>" + Glib::Markup::escape_text(std::string(branch_name)) + "</tt>");

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


        {
            Glib::ustring branch_name_u(branch_name);
            const auto regex_branch = Glib::Regex::create(R"(^pr-\d+-\w+$)");
            bool on_pr = regex_branch->match(branch_name_u);
            pr_button->set_sensitive(!on_pr);
            back_to_master_button->set_sensitive(on_pr);
            back_to_master_delete_button->set_sensitive(on_pr);
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

class SelectPRDialog : public Gtk::Dialog {
public:
    SelectPRDialog(Gtk::Window &parent);

    Gtk::SpinButton *pr_id_sp = nullptr;
};

SelectPRDialog::SelectPRDialog(Gtk::Window &parent)
    : Gtk::Dialog("Select Pull Request", parent,
                  Gtk::DialogFlags::DIALOG_MODAL | Gtk::DialogFlags::DIALOG_USE_HEADER_BAR)
{
    add_button("Cancel", Gtk::RESPONSE_CANCEL);
    add_button("Select", Gtk::RESPONSE_OK);
    set_default_response(Gtk::RESPONSE_OK);
    auto grid = Gtk::manage(new Gtk::Grid);
    grid->set_row_spacing(10);
    grid->set_column_spacing(10);
    grid->property_margin() = 20;
    grid->set_halign(Gtk::ALIGN_CENTER);

    int top = 0;
    pr_id_sp = Gtk::manage(new Gtk::SpinButton);
    pr_id_sp->set_range(0, 10000);
    pr_id_sp->signal_activate().connect([this] { response(Gtk::RESPONSE_OK); });
    grid_attach_label_and_widget(grid, "PR #", pr_id_sp, top);

    get_content_area()->add(*grid);
    get_content_area()->set_border_width(0);
    grid->show_all();
}

void PoolGitBox::handle_pr()
{
    unsigned int pr_id = 0;
    {
        SelectPRDialog dia(*notebook->appwin);
        if (dia.run() == Gtk::RESPONSE_OK) {
            pr_id = dia.pr_id_sp->get_value_as_int();
        }
        else {
            return;
        }
    }

    autofree_ptr<git_repository> repo(git_repository_free);
    if (git_repository_open(&repo.ptr, notebook->base_path.c_str()) != 0) {
        throw std::runtime_error("error opening repo");
    }

    autofree_ptr<git_remote> remote(git_remote_free);
    if (git_remote_lookup(&remote.ptr, repo, "origin") != 0) {
        throw std::runtime_error("error finding remote");
    }

    Glib::ustring remote_url(git_remote_url(remote));
    const auto regex_url = Glib::Regex::create(R"(^git@github\.com:([\w-]+)\/([\w-]+).git$)");
    Glib::MatchInfo ma;
    if (regex_url->match(remote_url, ma)) {
        auto gh_owner = ma.fetch(1);
        auto gh_repo = ma.fetch(2);
        std::cout << gh_owner << " " << gh_repo << std::endl;
        GitHubClient client;
        auto pr = client.get_pull_request(gh_owner, gh_repo, pr_id);
        std::cout << std::setw(4) << pr << std::endl;

        std::string pr_url = pr.at("head").at("repo").at("git_url");
        std::string pr_ref = pr.at("head").at("ref");
        std::string pr_remote_name = "pr-" + std::to_string(pr_id);
        std::string fetchspec = "+refs/heads/" + pr_ref + ":refs/remotes/" + pr_remote_name + "/pr";

        autofree_ptr<git_remote> pr_remote(git_remote_free);
        {
            auto rc = git_remote_lookup(&pr_remote.ptr, repo, pr_remote_name.c_str());
            if (rc == 0) {
                // okay
            }
            else if (rc == GIT_ENOTFOUND) {
                pr_remote.ptr = nullptr;
            }
            else {
                throw std::runtime_error("lookup");
            }
        }

        if (pr_remote.ptr == nullptr) {
            if (git_remote_create_with_fetchspec(&pr_remote.ptr, repo, pr_remote_name.c_str(), pr_url.c_str(),
                                                 fetchspec.c_str())
                != 0) {
                throw std::runtime_error("error creating remote");
            }
        }

        git_fetch_options opts = GIT_FETCH_OPTIONS_INIT;
        if (git_remote_fetch(pr_remote, NULL, &opts, NULL) != 0) {
            throw std::runtime_error("fetching");
        }

        autofree_ptr<git_object> pr_obj(git_object_free);
        if (git_revparse_single(&pr_obj.ptr, repo, (pr_remote_name + "/pr").c_str()) != 0) {
            throw std::runtime_error("revparse");
        }
        auto pr_oid = git_object_id(pr_obj);

        autofree_ptr<git_commit> pr_commit(git_commit_free);
        if (git_commit_lookup(&pr_commit.ptr, repo, pr_oid) != 0) {
            throw std::runtime_error("commit lookup");
        }
        std::cout << git_commit_message(pr_commit) << std::endl;
        {
            autofree_ptr<git_reference> pr_branch_orig(git_reference_free);
            if (git_branch_create(&pr_branch_orig.ptr, repo, (pr_remote_name + "-orig").c_str(), pr_commit, true)
                != 0) {
                throw std::runtime_error("upstream branch create");
            }
        }


        autofree_ptr<git_object> obj(git_object_free);
        if (git_revparse_single(&obj.ptr, repo, "master") != 0) {
            throw std::runtime_error("revparse");
        }
        auto oid = git_object_id(obj);
        autofree_ptr<git_commit> master_commit(git_commit_free);
        if (git_commit_lookup(&master_commit.ptr, repo, oid) != 0) {
            throw std::runtime_error("commit lookup");
        }
        std::cout << git_commit_message(master_commit) << std::endl;

        {

            autofree_ptr<git_reference> pr_branch_orig(git_reference_free);
            if (git_branch_create(&pr_branch_orig.ptr, repo, (pr_remote_name + "-merged").c_str(), master_commit, true)
                != 0) {
                throw std::runtime_error("branch create");
            }

            autofree_ptr<git_object> treeish(git_object_free);
            if (git_reference_peel(&treeish.ptr, pr_branch_orig, GIT_OBJ_TREE) != 0) {
                throw std::runtime_error("peel");
            }

            git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;
            checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE;
            if (git_checkout_tree(repo, treeish, &checkout_opts) != 0) {
                throw std::runtime_error("error checking out tree");
            }

            if (git_repository_set_head(repo, git_reference_name(pr_branch_orig)) != 0) {
                throw std::runtime_error("error setting head");
            }
        }

        {
            auto state = git_repository_state(repo);
            if (state != GIT_REPOSITORY_STATE_NONE) {
                throw std::runtime_error("state error");
            }
        }
        {
            autofree_ptr<git_annotated_commit> com(git_annotated_commit_free);
            if (git_annotated_commit_lookup(&com.ptr, repo, pr_oid) != 0)
                throw std::runtime_error("lookup");
            const git_annotated_commit *acom = com.ptr;

            git_merge_analysis_t merge_analysis;
            git_merge_preference_t merge_prefs = GIT_MERGE_PREFERENCE_NO_FASTFORWARD;
            if (git_merge_analysis(&merge_analysis, &merge_prefs, repo, &acom, 1) != 0) {
                throw std::runtime_error("error getting merge analysis");
            }
            if (merge_analysis != GIT_MERGE_ANALYSIS_NORMAL) {
                throw std::runtime_error("merge not normal");
            }

            git_merge_options merge_opts = GIT_MERGE_OPTIONS_INIT;
            git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;

            if (git_merge(repo, &acom, 1, &merge_opts, &checkout_opts) != 0) {
                throw std::runtime_error("merge failed");
            }
        }

        {
            autofree_ptr<git_index> index(git_index_free);
            if (git_repository_index(&index.ptr, repo) != 0) {
                throw std::runtime_error("index error");
            }
            if (git_index_has_conflicts(index)) {
                throw std::runtime_error("has conflicts");
            }

            git_oid tree_oid;

            if (git_index_write_tree(&tree_oid, index) != 0) {
                throw std::runtime_error("error saving index");
            }

            autofree_ptr<git_tree> tree(git_tree_free);
            if (git_tree_lookup(&tree.ptr, repo, &tree_oid) != 0) {
                throw std::runtime_error("error looking up tree");
            }

            autofree_ptr<git_signature> signature(git_signature_free);
            git_signature_default(&signature.ptr, repo);

            git_oid new_commit_oid;
            std::array<const git_commit *, 2> parents;
            parents.at(0) = master_commit;
            parents.at(1) = pr_commit;
            if (git_commit_create(&new_commit_oid, repo, "HEAD", signature, signature, NULL,
                                  ("merge pr " + std::to_string(pr_id)).c_str(), tree, parents.size(), parents.data())
                != 0) {
                throw std::runtime_error("error committing");
            }
            git_repository_state_cleanup(repo);
        }
        refresh();
        notebook->pool_update();
    }
}

static void remove_branch(git_repository *repo, const char *branch_name)
{
    autofree_ptr<git_reference> branch(git_reference_free);

    if (git_branch_lookup(&branch.ptr, repo, branch_name, GIT_BRANCH_LOCAL) != 0) {
        throw std::runtime_error("branch lookup");
    }

    if (git_branch_delete(branch) != 0) {
        throw std::runtime_error("branch delete");
    }
}

void PoolGitBox::handle_back_to_master(bool delete_pr)
{
    autofree_ptr<git_repository> repo(git_repository_free);
    if (git_repository_open(&repo.ptr, notebook->base_path.c_str()) != 0) {
        throw std::runtime_error("error opening repo");
    }

    Glib::ustring branch_name_u;
    {
        autofree_ptr<git_reference> head(git_reference_free);
        if (git_repository_head(&head.ptr, repo) != 0) {
            throw std::runtime_error("error getting head");
        }
        const char *branch_name = "fixme";
        if (git_branch_name(&branch_name, head) != 0) {
            throw std::runtime_error("error getting branch name");
        }

        branch_name_u = branch_name;
    }

    autofree_ptr<git_object> treeish(git_object_free);
    git_revparse_single(&treeish.ptr, repo, "master");

    git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;
    checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE;
    if (git_checkout_tree(repo, treeish, &checkout_opts) != 0) {
        throw std::runtime_error("error checking out tree");
    }

    if (git_repository_set_head(repo, "refs/heads/master") != 0) {
        throw std::runtime_error("error setting head");
    }

    if (delete_pr) {
        {
            std::cout << branch_name_u << std::endl;
            const auto regex_branch = Glib::Regex::create(R"(^pr-(\d+)-\w+$)");
            Glib::MatchInfo ma;
            if (regex_branch->match(branch_name_u, ma)) {
                const std::string pr_id = ma.fetch(1);
                std::cout << pr_id << std::endl;
                const std::string pr_branch_name_orig = "pr-" + pr_id + "-orig";
                const std::string pr_branch_name_merged = "pr-" + pr_id + "-merged";
                const std::string pr_remote_name = "pr-" + pr_id;

                if (git_remote_delete(repo, pr_remote_name.c_str()) != 0) {
                    throw std::runtime_error("error deleting remote");
                }
                remove_branch(repo, pr_branch_name_merged.c_str());
                remove_branch(repo, pr_branch_name_orig.c_str());
            }
        }
    }

    refresh();
    notebook->pool_update();
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
