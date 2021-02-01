#include "pool_remote_box.hpp"
#include <git2.h>
#include <git2/cred_helpers.h>
#include "util/autofree_ptr.hpp"
#include "util/gtk_util.hpp"
#include "pool_notebook.hpp"
#include "pool_merge_dialog.hpp"
#include <thread>
#include "pool-update/pool-update.hpp"
#include "common/object_descr.hpp"
#include "util/github_client.hpp"
#include "util/str_util.hpp"
#include "util/util.hpp"
#include "github_login_window.hpp"
#include <iomanip>
#include "util/win32_undef.hpp"
#include "check_column.hpp"
#include "checks/check_item.hpp"


namespace horizon {

class ConfirmPrDialog : public Gtk::Dialog {
public:
    ConfirmPrDialog(Gtk::Window *parent);

    Gtk::Entry *name_entry = nullptr;
    Gtk::Entry *email_entry = nullptr;
};

ConfirmPrDialog::ConfirmPrDialog(Gtk::Window *parent)
    : Gtk::Dialog("Confirm Pull Request", *parent,
                  Gtk::DialogFlags::DIALOG_MODAL | Gtk::DialogFlags::DIALOG_USE_HEADER_BAR)
{
    add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
    add_button("Confirm", Gtk::ResponseType::RESPONSE_OK);
    set_default_response(Gtk::RESPONSE_OK);
    auto grid = Gtk::manage(new Gtk::Grid);
    grid->set_row_spacing(10);
    grid->set_column_spacing(10);
    grid->set_halign(Gtk::ALIGN_CENTER);

    int top = 0;
    name_entry = Gtk::manage(new Gtk::Entry);
    grid_attach_label_and_widget(grid, "Your Name", name_entry, top);

    email_entry = Gtk::manage(new Gtk::Entry);
    grid_attach_label_and_widget(grid, "Your Email", email_entry, top);

    name_entry->signal_activate().connect([this] { email_entry->grab_focus(); });
    email_entry->signal_activate().connect([this] { response(Gtk::RESPONSE_OK); });

    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 20));
    box->property_margin() = 20;
    box->pack_start(*grid, true, true, 0);
    box->set_halign(Gtk::ALIGN_CENTER);
    box->set_hexpand(false);

    auto la = Gtk::manage(new Gtk::Label());
    la->set_markup(
            "This information will be used for the commit and will be visible "
            "publicly.");
    la->set_max_width_chars(10);
    la->set_line_wrap(true);
    la->set_line_wrap_mode(Pango::WRAP_WORD);
    la->set_xalign(0);
    la->get_style_context()->add_class("dim-label");
    la->set_line_wrap(true);
    box->pack_start(*la, false, false, 0);

    get_content_area()->add(*box);
    box->show_all();
}

static const std::string github_client_id = "094ac4cbd2e4a51820b4";

PoolRemoteBox::PoolRemoteBox(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, PoolNotebook *nb)
    : Gtk::Box(cobject), notebook(nb)
{
    x->get_widget("button_upgrade_pool", upgrade_button);
    x->get_widget("button_do_create_pr", create_pr_button);
    x->get_widget("upgrade_revealer", upgrade_revealer);
    x->get_widget("upgrade_spinner", upgrade_spinner);
    x->get_widget("upgrade_label", upgrade_label);
    x->get_widget("remote_gh_repo_link_label", gh_repo_link_label);
    x->get_widget("merge_items_view", merge_items_view);
    x->get_widget("pull_requests_listbox", pull_requests_listbox);
    x->get_widget("merge_items_placeholder_label", merge_items_placeholder_label);
    x->get_widget("pr_body_placeholder_label", pr_body_placeholder_label);
    x->get_widget("merge_items_clear_button", merge_items_clear_button);
    x->get_widget("merge_items_remove_button", merge_items_remove_button);
    x->get_widget("merge_items_run_checks_button", merge_items_run_checks_button);
    x->get_widget("button_refresh_pull_requests", refresh_prs_button);
    x->get_widget("remote_gh_signed_in_label", gh_signed_in_label);
    x->get_widget("remote_pr_title_entry", pr_title_entry);
    x->get_widget("remote_pr_body_textview", pr_body_textview);
    x->get_widget("pr_spinner", pr_spinner);
    x->get_widget("remote_login_button", login_button);
    x->get_widget("remote_logout_button", logout_button);
    x->get_widget("show_only_my_prs_cb", show_only_my_prs_cb);
    x->get_widget("pr_update_button", pr_update_button);
    x->get_widget("pr_update_cancel_button", pr_update_cancel_button);
    logout_button->hide();

    login_button->signal_clicked().connect([this] {
        if (Glib::file_test(get_token_filename(), Glib::FILE_TEST_IS_REGULAR) && login_succeeded) {
            update_login();
            return;
        }

        auto lw = GitHubLoginWindow::create(get_token_filename(), github_client_id);
        auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));
        lw->set_transient_for(*top);
        lw->set_modal(true);
        lw->signal_hide().connect([this, lw] {
            delete lw;
            update_login();
        });
        lw->present();
    });

    logout_button->signal_clicked().connect([this] {
        auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));
        Gtk::MessageDialog md(*top, "Log out?", false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_NONE, true);
        md.set_secondary_text(
                "Before logging out, you might want to <a "
                "href=\"https://github.com/settings/connections/applications/"
                        + github_client_id + "\">revoke</a> the authentication token.",
                true);
        md.add_button("Cancel", Gtk::RESPONSE_CANCEL);
        md.add_button("Log out", Gtk::RESPONSE_OK);
        md.set_default_response(Gtk::RESPONSE_CANCEL);
        if (md.run() == Gtk::RESPONSE_OK) {
            Gio::File::create_for_path(get_token_filename())->remove();
            gh_username.clear();
            update_my_prs();
            gh_token.clear();
            update_login();
            upgrade_label->set_text("");
        }
    });

    pr_update_cancel_button->signal_clicked().connect([this] { set_pr_update_mode(0, ""); });

    pull_requests_listbox->set_header_func(sigc::ptr_fun(header_func_separator));

    show_only_my_prs_cb->signal_toggled().connect(sigc::mem_fun(*this, &PoolRemoteBox::update_prs));

    autofree_ptr<git_repository> repo(git_repository_free);
    if (git_repository_open(&repo.ptr, notebook->remote_repo.c_str()) != 0) {
        throw std::runtime_error("error opening repo");
    }

    autofree_ptr<git_remote> remote(git_remote_free);
    if (git_remote_lookup(&remote.ptr, repo, "origin") != 0) {
        throw std::runtime_error("error finding remote");
    }

    Glib::ustring remote_url(git_remote_url(remote));
    const auto regex_url = Glib::Regex::create("^https:\\/\\/github.com\\/([\\w-]+)\\/([\\w-]+).git$");
    Glib::MatchInfo ma;
    if (regex_url->match(remote_url, ma)) {
        gh_owner = ma.fetch(1);
        gh_repo = ma.fetch(2);
        std::string markup = "<a href=\"https://github.com/" + gh_owner + "/" + gh_repo + "\">" + gh_owner + " / "
                             + gh_repo + "</a>";
        gh_repo_link_label->set_markup(markup);
    }
    else {
        gh_repo_link_label->set_text("couldn't find github repo!");
    }

    upgrade_button->signal_clicked().connect(sigc::mem_fun(*this, &PoolRemoteBox::handle_remote_upgrade));
    create_pr_button->signal_clicked().connect(sigc::mem_fun(*this, &PoolRemoteBox::handle_create_pr));
    pr_update_button->signal_clicked().connect(sigc::mem_fun(*this, &PoolRemoteBox::handle_update_pr));
    refresh_prs_button->signal_clicked().connect(sigc::mem_fun(*this, &PoolRemoteBox::handle_refresh_prs));

    pr_status_dispatcher.attach(pr_spinner);
    pr_status_dispatcher.signal_notified().connect([this](const StatusDispatcher::Notification &n) {
        auto is_busy = n.status == StatusDispatcher::Status::BUSY;
        if (!is_busy) {
            notebook->pool_updating = false;
        }
        if (n.status == StatusDispatcher::Status::DONE) {
            update_prs();
        }
    });

    git_thread_dispatcher.connect([this] {
        std::lock_guard<std::mutex> lock(git_thread_mutex);
        upgrade_revealer->set_reveal_child(git_thread_busy || git_thread_error);
        upgrade_spinner->property_active() = !git_thread_error;
        upgrade_label->set_text(git_thread_status);
        if (gh_username.size()) {
            gh_signed_in_label->set_text("Logged in: " + gh_username);
        }
        else {
            gh_signed_in_label->set_text("Not logged in");
        }
        login_button->set_visible(gh_username.size() == 0);
        logout_button->set_visible(gh_username.size() || login_succeeded == false);
        update_my_prs();
        if (!git_thread_busy) {
            notebook->pool_updating = false;
        }
        if (git_thread_mode == GitThreadMode::UPGRADE && !git_thread_busy && !git_thread_error) {
            auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));
            PoolMergeDialog dia(top, notebook->base_path, notebook->remote_repo);
            dia.run();
            if (dia.get_merged())
                notebook->pool_update();
        }
        else if (git_thread_mode == GitThreadMode::PULL_REQUEST && !git_thread_busy && !git_thread_error) {
            items_merge.clear();
            models_merge.clear();
            update_items_merge();
            pr_title_entry->set_text("");
            pr_body_textview->get_buffer()->set_text("");
            handle_refresh_prs();
        }
        else if (git_thread_mode == GitThreadMode::PULL_REQUEST_UPDATE && !git_thread_busy && !git_thread_error) {
            set_pr_update_mode(0, "");
            handle_refresh_prs();
        }
        else if (git_thread_mode == GitThreadMode::LOGIN && !git_thread_busy && !git_thread_error) {
            handle_refresh_prs();
        }
        else if (git_thread_mode == GitThreadMode::PULL_REQUEST_UPDATE_PREPARE && !git_thread_busy
                 && !git_thread_error) {
            set_pr_update_mode(pr_update_nr, pr_update_branch);
        }
    });


    item_store = Gtk::ListStore::create(list_columns);
    merge_items_view->set_model(item_store);

    {
        auto cr = Gtk::manage(new Gtk::CellRendererText());
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Type", *cr));
        tvc->set_cell_data_func(*cr, [this](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
            Gtk::TreeModel::Row row = *it;
            auto mcr = dynamic_cast<Gtk::CellRendererText *>(tcr);
            mcr->property_text() = object_descriptions.at(row[list_columns.type]).name;
        });
        merge_items_view->append_column(*tvc);
    }

    merge_items_view->signal_row_activated().connect(
            [this](const Gtk::TreeModel::Path &path, Gtk::TreeViewColumn *col) {
                Gtk::TreeModel::Row row = *item_store->get_iter(path);
                const ObjectType type = row[list_columns.type];
                if (type != ObjectType::MODEL_3D)
                    notebook->go_to(type, row[list_columns.uuid]);
            });

    merge_items_view->append_column("Name", list_columns.name);
    add_check_column(*merge_items_view, list_columns.check_result);

    merge_items_clear_button->signal_clicked().connect([this] {
        items_merge.clear();
        models_merge.clear();
        update_items_merge();
    });

    merge_items_remove_button->signal_clicked().connect([this] {
        auto it = merge_items_view->get_selection()->get_selected();
        if (it) {
            Gtk::TreeModel::Row row = *it;
            ObjectType type = row[list_columns.type];
            if (type == ObjectType::MODEL_3D) {
                models_merge.erase(row[list_columns.filename]);
            }
            else {
                std::pair<ObjectType, UUID> key(type, row[list_columns.uuid]);
                items_merge.erase(key);
            }
        }
        update_items_merge();
    });

    merge_items_run_checks_button->signal_clicked().connect(sigc::mem_fun(*this, &PoolRemoteBox::update_items_merge));

    merge_items_view->get_selection()->signal_changed().connect([this] {
        auto it = merge_items_view->get_selection()->get_selected();
        merge_items_remove_button->set_sensitive(it);
    });
    merge_items_remove_button->set_sensitive(false);

    pr_body_textview->signal_focus_in_event().connect([this](GdkEventFocus *ev) {
        update_body_placeholder_label();
        return false;
    });

    pr_body_textview->signal_focus_out_event().connect([this](GdkEventFocus *ev) {
        update_body_placeholder_label();
        return false;
    });

    pr_body_textview->get_buffer()->signal_changed().connect([this] { update_body_placeholder_label(); });

    set_pr_update_mode(0, "");
}

int diff_file_cb_c(const git_diff_delta *delta, float progress, void *pl)
{
    auto &diff_files = *reinterpret_cast<std::vector<std::string> *>(pl);
    if (delta->status == GIT_DELTA_ADDED || delta->status == GIT_DELTA_MODIFIED) {
        diff_files.push_back(delta->new_file.path);
    }
    return 0;
}

static std::pair<git_oid, autofree_ptr<git_tree>> find_base(git_repository *repo, git_commit *latest_commit)
{
    git_oid oid_base;
    autofree_ptr<git_tree> base_tree(git_tree_free);
    {
        autofree_ptr<git_object> master_obj(git_object_free);
        if (git_revparse_single(&master_obj.ptr, repo, "master") != 0) {
            throw std::runtime_error("error looking up master");
        }
        if (git_merge_base(&oid_base, repo, git_object_id(reinterpret_cast<git_object *>(latest_commit)),
                           git_object_id(master_obj))
            != 0) {
            throw std::runtime_error("error finding merge base");
        }

        autofree_ptr<git_commit> base_commit(git_commit_free);
        if (git_commit_lookup(&base_commit.ptr, repo, &oid_base) != 0) {
            throw std::runtime_error("error finding merge base commit");
        }
        if (git_commit_tree(&base_tree.ptr, base_commit) != 0) {
            throw std::runtime_error("error finding merge base tree");
        }
    }
    return {oid_base, std::move(base_tree)};
}

void PoolRemoteBox::pr_diff_file_cb(const git_diff_delta *delta)
{
    SQLite::Query q(notebook->pool.db, "INSERT INTO 'git_files' VALUES (?)");
    q.bind(1, std::string(delta->new_file.path));
    q.step();
}

int PoolRemoteBox::pr_diff_file_cb_c(const git_diff_delta *delta, float progress, void *pl)
{
    auto self = reinterpret_cast<PoolRemoteBox *>(pl);
    self->pr_diff_file_cb(delta);
    return 0;
}


void PoolRemoteBox::set_pr_update_mode(unsigned int pr, const std::string branch_name)
{
    if (!pr) {
        items_merge.clear();
        models_merge.clear();
        update_items_merge();
        pr_title_entry->set_text("");
        pr_body_textview->get_buffer()->set_text("");
        pr_update_button->set_visible(false);
        pr_update_cancel_button->set_visible(false);
        create_pr_button->set_visible(true);
        pr_update_nr = 0;
        update_prs();
        return;
    }

    autofree_ptr<git_repository> repo(git_repository_free);
    if (git_repository_open(&repo.ptr, notebook->remote_repo.c_str()) != 0) {
        throw std::runtime_error("error opening repo");
    }

    autofree_ptr<git_reference> branch(git_reference_free);
    if (const auto err = git_branch_lookup(&branch.ptr, repo, branch_name.c_str(), GIT_BRANCH_LOCAL);
        err == GIT_ENOTFOUND) {
        // branch not found

        if (notebook->pool_updating)
            return;

        notebook->pool_updating = true;
        git_thread_busy = true;
        git_thread_error = false;
        git_thread_mode = GitThreadMode::PULL_REQUEST_UPDATE_PREPARE;

        pr_update_nr = pr;
        pr_update_branch = branch_name;

        std::thread thr(&PoolRemoteBox::update_prepare_pr_thread, this);

        thr.detach();

        return;
    }
    else if (err != 0) {
        throw std::runtime_error("branch lookup: " + branch_name);
    }

    pr_update_button->set_visible(true);
    pr_update_cancel_button->set_visible(true);
    create_pr_button->set_visible(false);
    pr_update_button->set_label("Update PR #" + std::to_string(pr));
    pr_update_nr = pr;
    pr_update_branch = branch_name;
    update_prs();

    autofree_ptr<git_commit> latest_commit(git_commit_free);
    if (git_reference_peel(reinterpret_cast<git_object **>(&latest_commit.ptr), branch, GIT_OBJ_COMMIT) != 0) {
        throw std::runtime_error("branch peel");
    }

    pr_title_entry->set_text(git_commit_message(latest_commit));


    auto [oid_base, base_tree] = find_base(repo, latest_commit);

    autofree_ptr<git_tree> tree(git_tree_free);
    git_commit_tree(&tree.ptr, latest_commit);

    autofree_ptr<git_diff> diff(git_diff_free);
    git_diff_options diff_opts = GIT_DIFF_OPTIONS_INIT;
    git_diff_tree_to_tree(&diff.ptr, repo, base_tree, tree, &diff_opts);


    notebook->pool.db.execute("CREATE TEMP TABLE 'git_files' ('git_filename' TEXT NOT NULL);");
    notebook->pool.db.execute("BEGIN TRANSACTION");
    git_diff_foreach(diff, &PoolRemoteBox::pr_diff_file_cb_c, nullptr, nullptr, nullptr, this);
    notebook->pool.db.execute("COMMIT");

    SQLite::Query q(
            notebook->pool.db,
            "SELECT type, uuid, name, filename FROM git_files LEFT JOIN "
            "(SELECT type, uuid, name, filename FROM all_items_view UNION ALL SELECT DISTINCT 'model_3d' AS type, "
            "'00000000-0000-0000-0000-000000000000' as uuid, '' as name, model_filename as filename FROM models) "
            "ON filename=git_filename WHERE filename IS NOT NULL");
    while (q.step()) {
        const ObjectType type = object_type_lut.lookup(q.get<std::string>(0));
        const UUID uu = q.get<std::string>(1);
        const auto filename = q.get<std::string>(3);
        if (type == ObjectType::MODEL_3D) {
            models_merge.emplace(filename);
        }
        else {
            items_merge.emplace(type, uu);
        }
    }
    update_items_merge();

    notebook->pool.db.execute("DROP TABLE 'git_files'");
}

class PullRequestItemBox : public Gtk::Box {
public:
    PullRequestItemBox(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, const json &j,
                       const std::string &gh_user, bool no_update);
    static PullRequestItemBox *create(const json &j, const std::string &gh_user, bool no_update);
    const std::string gh_user;
    const int pr_number;
    const std::string branch_name;

public:
    typedef sigc::signal<void> type_signal_update_clicked;
    type_signal_update_clicked signal_update_clicked()
    {
        return s_signal_update_clicked;
    }

private:
    type_signal_update_clicked s_signal_update_clicked;
};

PullRequestItemBox::PullRequestItemBox(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, const json &j,
                                       const std::string &a_gh_user, bool no_update)
    : Gtk::Box(cobject), gh_user(a_gh_user), pr_number(j.at("number")), branch_name(j.at("head").at("ref"))
{
    Gtk::Label *name_label, *descr_label, *number_label;
    Gtk::LinkButton *link_button;
    Gtk::Button *update_button;
    x->get_widget("pull_request_item_name_label", name_label);
    x->get_widget("pull_request_item_descr_label", descr_label);
    x->get_widget("pull_request_item_number_label", number_label);
    x->get_widget("pull_request_item_link", link_button);
    x->get_widget("pull_request_update_button", update_button);

    update_button->signal_clicked().connect([this] { s_signal_update_clicked.emit(); });

    name_label->set_text(j.at("title").get<std::string>());
    number_label->set_text("#" + std::to_string(pr_number));
    link_button->set_uri(j.at("_links").at("html").at("href").get<std::string>());
    const std::string open_user = j.at("user").at("login");

    const bool is_pool_mgr_branch = Glib::Regex::match_simple(
            "^pool_mgr_[0-9a-f]{8}-[0-9a-f]{4}-[1-5][0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-"
            "f]{12}$",
            branch_name);

    update_button->set_visible(!no_update && open_user == gh_user && is_pool_mgr_branch);
    const std::string created_at = j.at("created_at");
    descr_label->set_text("opened by " + open_user + " on " + created_at);
}

PullRequestItemBox *PullRequestItemBox::create(const json &j, const std::string &gh_user, bool no_update)
{
    PullRequestItemBox *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/horizon-eda/horizon/pool-prj-mgr/window.ui", "pull_request_item");
    x->get_widget_derived("pull_request_item", w, j, gh_user, no_update);
    w->reference();
    return w;
}

void PoolRemoteBox::update_my_prs()
{
    if (gh_username.size()) {
        if (!show_only_my_prs_cb->get_sensitive())
            show_only_my_prs_cb->set_active(true);
        show_only_my_prs_cb->set_sensitive(true);
    }
    else {
        show_only_my_prs_cb->set_active(false);
        show_only_my_prs_cb->set_sensitive(false);
    }
}

void PoolRemoteBox::update_prs()
{
    {
        auto children = pull_requests_listbox->get_children();
        for (auto it : children) {
            delete it;
        }
    }

    for (const auto &it : pull_requests) {
        if (gh_username.size() && show_only_my_prs_cb->get_active()) {
            std::string user = it.at("user").at("login");
            if (user != gh_username)
                continue;
        }

        auto box = PullRequestItemBox::create(it, gh_username, pr_update_nr != 0);
        box->signal_update_clicked().connect([this, box] { set_pr_update_mode(box->pr_number, box->branch_name); });
        pull_requests_listbox->append(*box);
        box->show();
        box->unreference();
    }
}

void PoolRemoteBox::merge_item(ObjectType ty, const UUID &uu)
{
    items_merge.emplace(ty, uu);
    auto other_items = get_referenced(ty, uu);
    items_merge.insert(other_items.begin(), other_items.end());
    update_items_merge();
}

void PoolRemoteBox::merge_3d_model(const std::string &filename)
{
    models_merge.emplace(filename);
    update_items_merge();
}

bool PoolRemoteBox::exists_in_pool(Pool &pool, ObjectType ty, const UUID &uu)
{
    try {
        pool.get_filename(ty, uu);
        return true;
    }
    catch (...) {
        return false;
    }
}

ItemSet PoolRemoteBox::get_referenced(ObjectType ty, const UUID &uu)
{
    ItemSet items;
    items.emplace(ty, uu);
    Pool pool_remote(notebook->remote_repo);
    bool added = true;
    while (added) {
        added = false;
        for (const auto &it : items) {
            switch (it.first) {
            case ObjectType::ENTITY: {
                auto entity = notebook->pool.get_entity(it.second);
                for (const auto &it_gate : entity->gates) {
                    const Unit *unit = it_gate.second.unit;
                    if (!exists_in_pool(pool_remote, ObjectType::UNIT, unit->uuid)) {
                        if (items.emplace(ObjectType::UNIT, unit->uuid).second)
                            added = true;
                    }
                }
            } break;

            case ObjectType::SYMBOL: {
                auto symnol = notebook->pool.get_symbol(it.second);
                const Unit *unit = symnol->unit;
                if (!exists_in_pool(pool_remote, ObjectType::UNIT, unit->uuid)) {
                    if (items.emplace(ObjectType::UNIT, unit->uuid).second)
                        added = true;
                }
            } break;

            case ObjectType::PART: {
                auto part = notebook->pool.get_part(it.second);
                const Entity *entity = part->entity;
                const Package *package = part->package;
                if (!exists_in_pool(pool_remote, ObjectType::ENTITY, entity->uuid)) {
                    if (items.emplace(ObjectType::ENTITY, entity->uuid).second)
                        added = true;
                }
                if (!exists_in_pool(pool_remote, ObjectType::PACKAGE, package->uuid)) {
                    if (items.emplace(ObjectType::PACKAGE, package->uuid).second)
                        added = true;
                }
            } break;

            case ObjectType::PACKAGE: {
                auto package = notebook->pool.get_package(it.second);
                for (const auto &it_pad : package->pads) {
                    const Padstack *padstack = it_pad.second.pool_padstack;
                    if (!exists_in_pool(pool_remote, ObjectType::PADSTACK, padstack->uuid)) {
                        if (items.emplace(ObjectType::PADSTACK, padstack->uuid).second)
                            added = true;
                    }
                }
                const Package *alt_pkg = package->alternate_for;
                if (alt_pkg && !exists_in_pool(pool_remote, ObjectType::PACKAGE, alt_pkg->uuid)) {
                    if (items.emplace(ObjectType::PACKAGE, alt_pkg->uuid).second)
                        added = true;
                }
            } break;

            default:;
            }
            if (added)
                break;
        }
    }
    return items;
}

PoolRemoteBox *PoolRemoteBox::create(PoolNotebook *nb)
{
    PoolRemoteBox *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    std::vector<Glib::ustring> widgets = {"box_remote", "sg_remote"};
    x->add_from_resource("/org/horizon-eda/horizon/pool-prj-mgr/window.ui", widgets);
    x->get_widget_derived("box_remote", w, nb);
    w->reference();
    return w;
}

void PoolRemoteBox::handle_remote_upgrade()
{
    if (notebook->pool_updating)
        return;
    notebook->pool_updating = true;
    git_thread_busy = true;
    git_thread_error = false;
    git_thread_mode = GitThreadMode::UPGRADE;

    std::thread thr(&PoolRemoteBox::remote_upgrade_thread, this);

    thr.detach();
}

void PoolRemoteBox::handle_refresh_prs()
{
    if (notebook->pool_updating)
        return;
    notebook->pool_updating = true;

    pr_status_dispatcher.reset("Starting…");
    std::thread thr(&PoolRemoteBox::refresh_prs_thread, this);

    thr.detach();
}

void PoolRemoteBox::handle_create_pr()
{
    if (notebook->pool_updating)
        return;

    auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));

    { // confirm user name
        autofree_ptr<git_repository> repo(git_repository_free);
        if (git_repository_open(&repo.ptr, notebook->remote_repo.c_str()) != 0) {
            throw std::runtime_error("error opening repo");
        }

        autofree_ptr<git_config> repo_config_snapshot(git_config_free);
        git_repository_config_snapshot(&repo_config_snapshot.ptr, repo);

        ConfirmPrDialog dia(top);
        const char *user_name = nullptr;
        const char *user_email = nullptr;

        git_config_get_string(&user_name, repo_config_snapshot, "user.name");
        git_config_get_string(&user_email, repo_config_snapshot, "user.email");

        if (user_name)
            dia.name_entry->set_text(user_name);

        if (user_email)
            dia.email_entry->set_text(user_email);

        if (dia.run() != Gtk::RESPONSE_OK) {
            return;
        }

        autofree_ptr<git_config> repo_config(git_config_free);
        git_repository_config(&repo_config.ptr, repo);

        git_config_set_string(repo_config, "user.name", dia.name_entry->get_text().c_str());
        git_config_set_string(repo_config, "user.email", dia.email_entry->get_text().c_str());
    }

    notebook->pool_updating = true;
    git_thread_busy = true;
    git_thread_error = false;
    git_thread_mode = GitThreadMode::PULL_REQUEST;

    pr_body = pr_body_textview->get_buffer()->get_text();
    pr_title = pr_title_entry->get_text();
    trim(pr_title);
    trim(pr_body);

    std::thread thr(&PoolRemoteBox::create_pr_thread, this);

    thr.detach();
}

void PoolRemoteBox::handle_update_pr()
{
    if (notebook->pool_updating)
        return;

    if (!pr_update_nr)
        return;

    notebook->pool_updating = true;
    git_thread_busy = true;
    git_thread_error = false;
    git_thread_mode = GitThreadMode::PULL_REQUEST_UPDATE;

    pr_body = pr_body_textview->get_buffer()->get_text();
    pr_title = pr_title_entry->get_text();
    trim(pr_title);
    trim(pr_body);

    std::thread thr(&PoolRemoteBox::update_pr_thread, this);

    thr.detach();
}

void PoolRemoteBox::update_items_merge()
{
    item_store->clear();
    auto can_merge = items_merge.size() > 0 || models_merge.size() > 0;
    create_pr_button->set_sensitive(can_merge);
    merge_items_clear_button->set_sensitive(can_merge);
    merge_items_run_checks_button->set_sensitive(can_merge);
    merge_items_placeholder_label->set_visible(items_merge.size() == 0 && models_merge.size() == 0);
    for (const auto &it : items_merge) {
        SQLite::Query q(notebook->pool.db, "SELECT name FROM all_items_view WHERE uuid = ? AND type = ?");
        q.bind(1, it.second);
        q.bind(2, object_type_lut.lookup_reverse(it.first));
        if (q.step()) {
            Gtk::TreeModel::Row row = *item_store->append();
            row[list_columns.name] = q.get<std::string>(0);
            row[list_columns.type] = it.first;
            row[list_columns.uuid] = it.second;
            auto result = check_item(notebook->pool, it.first, it.second);
            row[list_columns.check_result] = result;
        }
    }

    for (const auto &it : models_merge) {
        Gtk::TreeModel::Row row = *item_store->append();
        row[list_columns.name] = Glib::path_get_basename(it);
        row[list_columns.type] = ObjectType::MODEL_3D;
        row[list_columns.filename] = it;
    }

    if (notebook->page_num(*this) != -1) {
        const auto n = items_merge.size() + models_merge.size();
        if (n > 0) {
            notebook->set_tab_label_text(*this, "Remote (" + std::to_string(n) + ")");
        }
        else {
            notebook->set_tab_label_text(*this, "Remote");
        }
    }
}

void PoolRemoteBox::update_body_placeholder_label()
{
    bool has_text = pr_body_textview->get_buffer()->get_text().size();
    bool has_focus = pr_body_textview->has_focus();
    pr_body_placeholder_label->set_visible(!(has_focus || has_text));
}

void PoolRemoteBox::refresh_prs_thread()
{
    try {
        pr_status_dispatcher.set_status(StatusDispatcher::Status::BUSY, "Refreshing");
        GitHubClient client;
        pull_requests = client.get_pull_requests(gh_owner, gh_repo);
        std::cout << "get prs" << std::endl;
        /*pull_requests = json::array();
        {
                json j;
                j["number"] = 42;
                j["title"] = "new part";
                j["created_at"] = "2011-01-26T19:01:12Z";
                pull_requests.push_back(j);
        }*/

        pr_status_dispatcher.set_status(StatusDispatcher::Status::DONE, "Done");
    }
    catch (const std::exception &e) {
        pr_status_dispatcher.set_status(StatusDispatcher::Status::ERROR, std::string("Error: ") + e.what());
    }
}

void PoolRemoteBox::remote_upgrade_thread()
{
    try {
        {
            std::lock_guard<std::mutex> lock(git_thread_mutex);
            git_thread_status = "Opening repository…";
        }
        git_thread_dispatcher.emit();

        autofree_ptr<git_repository> repo(git_repository_free);
        if (git_repository_open(&repo.ptr, notebook->remote_repo.c_str()) != 0) {
            throw std::runtime_error("error opening repo");
        }

        autofree_ptr<git_remote> remote(git_remote_free);
        if (git_remote_lookup(&remote.ptr, repo, "origin") != 0) {
            throw std::runtime_error("error looking up remote");
        }

        {
            std::lock_guard<std::mutex> lock(git_thread_mutex);
            git_thread_status = "Fetching…";
        }
        git_thread_dispatcher.emit();

        git_fetch_options fetch_opts = GIT_FETCH_OPTIONS_INIT;
        if (git_remote_fetch(remote, NULL, &fetch_opts, NULL) != 0) {
            throw std::runtime_error("error fetching");
        }

        autofree_ptr<git_annotated_commit> latest_commit(git_annotated_commit_free);
        if (git_annotated_commit_from_revspec(&latest_commit.ptr, repo, "origin/master") != 0) {
            throw std::runtime_error("error getting latest commit ");
        }

        auto oid = git_annotated_commit_id(latest_commit);

        git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;
        checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE;

        const git_annotated_commit *com = latest_commit.ptr;
        git_merge_analysis_t merge_analysis;
        git_merge_preference_t merge_prefs = GIT_MERGE_PREFERENCE_FASTFORWARD_ONLY;
        if (git_merge_analysis(&merge_analysis, &merge_prefs, repo, &com, 1) != 0) {
            throw std::runtime_error("error getting merge analysis");
        }

        if (!(merge_analysis & GIT_MERGE_ANALYSIS_UP_TO_DATE)) {
            if (!(merge_analysis & GIT_MERGE_ANALYSIS_FASTFORWARD)) {
                throw std::runtime_error("can't fast-forward");
            }

            autofree_ptr<git_object> obj(git_object_free);
            if (git_object_lookup(&obj.ptr, repo, oid, GIT_OBJ_COMMIT) != 0) {
                throw std::runtime_error("error lookup");
            }

            if (git_checkout_tree(repo, obj, &checkout_opts) != 0) {
                throw std::runtime_error("error checkout");
            }

            autofree_ptr<git_reference> ref_head(git_reference_free);
            if (git_repository_head(&ref_head.ptr, repo) != 0) {
                throw std::runtime_error("error head");
            }

            autofree_ptr<git_reference> ref_new(git_reference_free);
            if (git_reference_set_target(&ref_new.ptr, ref_head, oid, "test") != 0) {
                throw std::runtime_error("error target");
            }
        }
        else {
            std::cout << "up to date" << std::endl;
        }

        {
            std::lock_guard<std::mutex> lock(git_thread_mutex);
            git_thread_status = "Updating remote pool…";
        }
        git_thread_dispatcher.emit();
        pool_update(notebook->remote_repo);

        {
            std::lock_guard<std::mutex> lock(git_thread_mutex);
            git_thread_status = "Updating local pool…";
        }
        git_thread_dispatcher.emit();
        unsigned int n_errors = 0;
        pool_update(notebook->base_path,
                    [&n_errors](PoolUpdateStatus status, std::string filename, std::string message) {
                        if (status == PoolUpdateStatus::FILE_ERROR) {
                            n_errors++;
                        }
                    });
        if (n_errors) {
            throw std::runtime_error("local pool update encountered errors");
        }

        {
            std::lock_guard<std::mutex> lock(git_thread_mutex);
            git_thread_busy = false;
            git_thread_status = "Done";
        }
        git_thread_dispatcher.emit();
    }
    catch (const std::exception &e) {
        {
            std::lock_guard<std::mutex> lock(git_thread_mutex);
            git_thread_busy = false;
            git_thread_error = true;
            git_thread_status = "Error: " + std::string(e.what());
        }
        git_thread_dispatcher.emit();
    }
}

std::string PoolRemoteBox::get_token_filename() const
{
    return Glib::build_filename(notebook->remote_repo, ".auth.json");
}

bool PoolRemoteBox::update_login()
{
    if (gh_username.size())
        return false;
    if (git_thread_busy)
        return false;
    const auto auth_avail = Glib::file_test(get_token_filename(), Glib::FILE_TEST_IS_REGULAR);
    if (!auth_avail) {
        gh_signed_in_label->set_text("Not logged in");
        login_button->show();
        logout_button->hide();
    }
    else {
        if (notebook->pool_updating)
            return false;
        notebook->pool_updating = true;
        git_thread_busy = true;
        git_thread_error = false;
        git_thread_mode = GitThreadMode::LOGIN;

        std::thread thr(&PoolRemoteBox::login_thread, this);

        thr.detach();
        return true;
    }
    return false;
}

void PoolRemoteBox::login_once()
{
    if (logged_in_once)
        return;

    if (!update_login())
        handle_refresh_prs();
    logged_in_once = true;
}

void PoolRemoteBox::login_thread()
{
    try {
        {
            std::lock_guard<std::mutex> lock(git_thread_mutex);
            git_thread_status = "Logging in…";
        }
        git_thread_dispatcher.emit();

        auto j_auth = load_json_from_file(get_token_filename());

        auto token = j_auth.at("token");

        GitHubClient client;
        auto user_info = client.login_token(token);

        gh_username = user_info.at("login");
        gh_token = token;

        {
            std::lock_guard<std::mutex> lock(git_thread_mutex);
            git_thread_busy = false;
            git_thread_status = "Logged in";
        }
        login_succeeded = true;
        git_thread_dispatcher.emit();
    }
    catch (const std::exception &e) {
        {
            std::lock_guard<std::mutex> lock(git_thread_mutex);
            login_succeeded = false;
            git_thread_busy = false;
            git_thread_error = true;
            git_thread_status = "Error: " + std::string(e.what());
        }
        git_thread_dispatcher.emit();
    }
}

void PoolRemoteBox::checkout_master(git_repository *repo)
{
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
}

struct PushPayload {
    git_cred_userpass_payload userpass;
    std::string ref_status;
};

static int push_cred_cb(git_cred **out, const char *url, const char *user_from_url, unsigned int allowed_types,
                        void *payload)
{
    auto pl = reinterpret_cast<PushPayload *>(payload);
    return git_cred_userpass(out, url, user_from_url, allowed_types, &pl->userpass);
}

static int push_ref_cb(const char *refname, const char *status, void *data)
{
    auto pl = reinterpret_cast<PushPayload *>(data);
    if (status)
        pl->ref_status += std::string(refname) + ":" + std::string(status) + " ";
    return 0;
}

git_oid PoolRemoteBox::items_to_tree(git_repository *repo)
{
    autofree_ptr<git_index> index(git_index_free);
    if (git_repository_index(&index.ptr, repo) != 0) {
        throw std::runtime_error("index error");
    }

    std::set<std::string> files_merge;

    for (const auto &it : items_merge) {
        SQLite::Query q(notebook->pool.db,
                        "SELECT filename FROM all_items_view WHERE "
                        "uuid = ? AND type = ?");
        q.bind(1, it.second);
        q.bind(2, object_type_lut.lookup_reverse(it.first));
        if (q.step()) {
            std::string filename = q.get<std::string>(0);
            files_merge.insert(filename);
        }
    }
    files_merge.insert(models_merge.begin(), models_merge.end());

    for (std::string filename : files_merge) {
        std::cout << "merge " << filename << std::endl;
        std::string filename_src = Glib::build_filename(notebook->base_path, filename);
        std::string filename_dest = Glib::build_filename(notebook->remote_repo, filename);
        std::string dirname_dest = Glib::path_get_dirname(filename_dest);
        if (!Glib::file_test(dirname_dest, Glib::FILE_TEST_IS_DIR))
            Gio::File::create_for_path(dirname_dest)->make_directory_with_parents();
        Gio::File::create_for_path(filename_src)
                ->copy(Gio::File::create_for_path(filename_dest), Gio::FILE_COPY_OVERWRITE);

#ifdef G_OS_WIN32
        replace_backslash(filename);
#endif

        if (git_index_add_bypath(index, filename.c_str()) != 0) {
            auto last_error = giterr_last();
            std::string err_str = last_error->message;
            throw std::runtime_error("add error: " + err_str);
        }
    }

    if (git_index_write(index) != 0) {
        throw std::runtime_error("error saving index");
    }

    git_oid tree_oid;
    if (git_index_write_tree(&tree_oid, index) != 0) {
        throw std::runtime_error("error saving index");
    }

    return tree_oid;
}

void PoolRemoteBox::push_branch(git_remote *remote, const std::string &branch_name)
{
    git_push_options push_opts = GIT_PUSH_OPTIONS_INIT;
    push_opts.callbacks.credentials = push_cred_cb;
    push_opts.callbacks.push_update_reference = push_ref_cb;
    PushPayload payload;
    payload.userpass.username = gh_username.c_str();
    payload.userpass.password = gh_token.c_str();
    push_opts.callbacks.payload = &payload;

    std::string refspec = "refs/heads/" + branch_name; // + ":refs/heads/" + new_branch_name;
    char *refspec_c = strdup(refspec.c_str());

    const git_strarray refs = {&refspec_c, 1};

    if (git_remote_push(remote, &refs, &push_opts) != 0) {
        auto err = giterr_last();
        std::string errstr = err->message;
        free(refspec_c);
        throw std::runtime_error("error pushing " + errstr);
    }
    free(refspec_c);
    if (payload.ref_status.size()) {
        throw std::runtime_error("error pushing " + payload.ref_status);
    }
}

autofree_ptr<git_remote> PoolRemoteBox::get_or_create_remote(GitHubClient &client, git_repository *repo)
{
    autofree_ptr<git_remote> my_remote(git_remote_free);
    if (const auto err = git_remote_lookup(&my_remote.ptr, repo, gh_username.c_str()); err == GIT_ENOTFOUND) {
        // remote doesn't exist, fork upstream
        auto fork_info = client.create_fork(gh_owner, gh_repo);
        int retries = 60;
        json forked_repo;
        while (retries) {
            try {
                forked_repo = client.get_repo(gh_username, gh_repo);
                break;
            }
            catch (const std::exception &e) {
                retries--;
                {
                    std::lock_guard<std::mutex> lock(git_thread_mutex);
                    git_thread_status = "Waiting for fork… (" + std::string(e.what()) + ")";
                }
                git_thread_dispatcher.emit();
                Glib::usleep(1e6);
            }
            catch (...) {
                retries--;
                {
                    std::lock_guard<std::mutex> lock(git_thread_mutex);
                    git_thread_status = "Waiting for fork…";
                }
                git_thread_dispatcher.emit();
                Glib::usleep(1e6);
            }
        }
        if (retries == 0) {
            throw std::runtime_error("timeout waiting for fork");
        }

        std::string fork_url = fork_info.at("clone_url");
        if (git_remote_create(&my_remote.ptr, repo, gh_username.c_str(), fork_url.c_str()) != 0) {
            throw std::runtime_error("error adding remote");
        }
        return my_remote;
    }
    else if (err == 0) {
        return my_remote;
    }
    else {
        throw std::runtime_error("error looking up remote");
    }
}

void PoolRemoteBox::create_pr_thread()
{
    try {

        {
            std::lock_guard<std::mutex> lock(git_thread_mutex);
            git_thread_status = "Authenticating…";
        }
        git_thread_dispatcher.emit();

        if (gh_token.size() == 0) {
            throw std::runtime_error("not logged in");
        }

        GitHubClient client;
        try {
            client.login_token(gh_token);
            git_thread_dispatcher.emit();
        }
        catch (const std::runtime_error &e) {
            gh_username = "";
            throw std::runtime_error("can't log in");
        }

        if (items_merge.size() == 0 && models_merge.size() == 0) {
            throw std::runtime_error("no items to merge");
        }

        if (pr_title.size() == 0) {
            throw std::runtime_error("please enter title");
        }

        if (pr_body.size() == 0) {
            throw std::runtime_error("please enter body");
        }

        {
            std::lock_guard<std::mutex> lock(git_thread_mutex);
            git_thread_status = "Opening repository…";
        }
        git_thread_dispatcher.emit();

        autofree_ptr<git_repository> repo(git_repository_free);
        if (git_repository_open(&repo.ptr, notebook->remote_repo.c_str()) != 0) {
            throw std::runtime_error("error opening repo");
        }

        auto my_remote = get_or_create_remote(client, repo);

        checkout_master(repo);


        autofree_ptr<git_annotated_commit> latest_annotated_commit(git_annotated_commit_free);
        if (git_annotated_commit_from_revspec(&latest_annotated_commit.ptr, repo, "origin/master") != 0) {
            throw std::runtime_error("error getting latest commit ");
        }

        auto oid = git_annotated_commit_id(latest_annotated_commit);
        autofree_ptr<git_commit> latest_commit(git_commit_free);
        if (git_commit_lookup(&latest_commit.ptr, repo, oid) != 0) {
            throw std::runtime_error("error looking up commit");
        }

        std::string new_branch_name = "pool_mgr_" + ((std::string)UUID::random());
        {
            autofree_ptr<git_reference> new_branch(git_reference_free);
            if (git_branch_create(&new_branch.ptr, repo, new_branch_name.c_str(), latest_commit, false) != 0) {
                throw std::runtime_error("error creating branch");
            }

            if (git_repository_set_head(repo, git_reference_name(new_branch)) != 0) {
                throw std::runtime_error("error setting head");
            }
        }

        git_oid tree_oid = items_to_tree(repo);

        {
            autofree_ptr<git_tree> tree(git_tree_free);
            if (git_tree_lookup(&tree.ptr, repo, &tree_oid) != 0) {
                throw std::runtime_error("error looking up tree");
            }


            git_oid parent_oid;
            git_reference_name_to_id(&parent_oid, repo, "HEAD");

            autofree_ptr<git_commit> parent_commit(git_commit_free);
            git_commit_lookup(&parent_commit.ptr, repo, &parent_oid);
            const git_commit *parent_commit_p = parent_commit.ptr;


            autofree_ptr<git_signature> signature(git_signature_free);
            git_signature_default(&signature.ptr, repo);

            git_oid new_commit_oid;
            if (git_commit_create(&new_commit_oid, repo, "HEAD", signature, signature, "UTF-8", pr_title.c_str(), tree,
                                  1, &parent_commit_p)
                != 0) {
                throw std::runtime_error("error committing");
            }
        }

        {
            std::lock_guard<std::mutex> lock(git_thread_mutex);
            git_thread_status = "Pushing…";
        }
        git_thread_dispatcher.emit();

        checkout_master(repo);

        push_branch(my_remote, new_branch_name);

        {
            std::lock_guard<std::mutex> lock(git_thread_mutex);
            git_thread_status = "Creating pull request…";
        }
        git_thread_dispatcher.emit();

        client.create_pull_request(gh_owner, gh_repo, pr_title, new_branch_name, "master", pr_body);


        {
            std::lock_guard<std::mutex> lock(git_thread_mutex);
            git_thread_busy = false;
            git_thread_status = "Done";
        }
        git_thread_dispatcher.emit();
    }
    catch (const std::exception &e) {
        {
            std::lock_guard<std::mutex> lock(git_thread_mutex);
            git_thread_busy = false;
            git_thread_error = true;
            git_thread_status = "Error: " + std::string(e.what());
        }
        git_thread_dispatcher.emit();
    }
    catch (const Gio::Error &e) {
        {
            std::lock_guard<std::mutex> lock(git_thread_mutex);
            git_thread_busy = false;
            git_thread_error = true;
            git_thread_status = "IO Error: " + std::string(e.what());
        }
        git_thread_dispatcher.emit();
    }
}

void PoolRemoteBox::update_pr_thread()
{
    try {

        {
            std::lock_guard<std::mutex> lock(git_thread_mutex);
            git_thread_status = "Authenticating…";
        }
        git_thread_dispatcher.emit();

        if (gh_token.size() == 0) {
            throw std::runtime_error("not logged in");
        }

        GitHubClient client;
        try {
            client.login_token(gh_token);
            git_thread_dispatcher.emit();
        }
        catch (const std::runtime_error &e) {
            gh_username = "";
            throw std::runtime_error("can't log in");
        }

        if (items_merge.size() == 0 && models_merge.size() == 0) {
            throw std::runtime_error("no items to merge");
        }

        if (pr_title.size() == 0) {
            throw std::runtime_error("please enter title");
        }


        {
            std::lock_guard<std::mutex> lock(git_thread_mutex);
            git_thread_status = "Opening repository…";
        }
        git_thread_dispatcher.emit();

        autofree_ptr<git_repository> repo(git_repository_free);
        if (git_repository_open(&repo.ptr, notebook->remote_repo.c_str()) != 0) {
            throw std::runtime_error("error opening repo");
        }

        autofree_ptr<git_remote> my_remote(git_remote_free);
        if (git_remote_lookup(&my_remote.ptr, repo, gh_username.c_str()) != 0) {
            throw std::runtime_error("can't find remote");
        }


        autofree_ptr<git_reference> branch(git_reference_free);
        if (git_branch_lookup(&branch.ptr, repo, pr_update_branch.c_str(), GIT_BRANCH_LOCAL) != 0) {
            throw std::runtime_error("branch lookup");
        }

        autofree_ptr<git_commit> latest_commit(git_commit_free);
        if (git_reference_peel(reinterpret_cast<git_object **>(&latest_commit.ptr), branch, GIT_OBJ_COMMIT) != 0) {
            throw std::runtime_error("branch peel");
        }

        auto [oid_base, base_tree] = find_base(repo, latest_commit);

        {
            git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;
            checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE;
            if (git_checkout_tree(repo, reinterpret_cast<git_object *>(base_tree.ptr), &checkout_opts) != 0) {
                throw std::runtime_error("error checking out tree");
            }
        }
        if (git_repository_set_head_detached(repo, &oid_base) != 0) {
            throw std::runtime_error("error setting detached head");
        }

        {
            auto tree_oid = items_to_tree(repo);
            autofree_ptr<git_tree> tree(git_tree_free);
            if (git_tree_lookup(&tree.ptr, repo, &tree_oid) != 0) {
                throw std::runtime_error("error looking up tree");
            }

            autofree_ptr<git_signature> signature(git_signature_free);
            git_signature_default(&signature.ptr, repo);

            const git_commit *parent_commit_p = latest_commit.ptr;

            git_oid new_commit_oid;
            if (git_commit_create(&new_commit_oid, repo, NULL, signature, signature, "UTF-8", pr_title.c_str(), tree, 1,
                                  &parent_commit_p)
                != 0) {
                throw std::runtime_error("error committing");
            }

            git_reference *new_ref = nullptr;
            if (git_reference_set_target(&new_ref, branch, &new_commit_oid, "update pr") != 0) {
                throw std::runtime_error("error updating branch");
            }
        }
        checkout_master(repo);
        {
            autofree_ptr<git_object> obj(git_object_free);
            git_revparse_single(&obj.ptr, repo, "master");

            git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;
            if (git_reset(repo, obj, GIT_RESET_HARD, &checkout_opts) != 0) {
                throw std::runtime_error("error resetting");
            }
        }


        {
            std::lock_guard<std::mutex> lock(git_thread_mutex);
            git_thread_status = "Pushing…";
        }
        git_thread_dispatcher.emit();

        push_branch(my_remote, pr_update_branch);

        if (pr_body.size()) {
            {
                std::lock_guard<std::mutex> lock(git_thread_mutex);
                git_thread_status = "Adding comment…";
            }
            git_thread_dispatcher.emit();
            client.add_issue_comment(gh_owner, gh_repo, pr_update_nr, pr_body);
        }

        {
            std::lock_guard<std::mutex> lock(git_thread_mutex);
            git_thread_busy = false;
            git_thread_status = "Done";
        }
        git_thread_dispatcher.emit();
    }
    catch (const std::exception &e) {
        {
            std::lock_guard<std::mutex> lock(git_thread_mutex);
            git_thread_busy = false;
            git_thread_error = true;
            git_thread_status = "Error: " + std::string(e.what());
        }
        git_thread_dispatcher.emit();
    }
    catch (const Gio::Error &e) {
        {
            std::lock_guard<std::mutex> lock(git_thread_mutex);
            git_thread_busy = false;
            git_thread_error = true;
            git_thread_status = "IO Error: " + std::string(e.what());
        }
        git_thread_dispatcher.emit();
    }
}


void PoolRemoteBox::update_prepare_pr_thread()
{
    try {

        {
            std::lock_guard<std::mutex> lock(git_thread_mutex);
            git_thread_status = "Authenticating…";
        }
        git_thread_dispatcher.emit();

        if (gh_token.size() == 0) {
            throw std::runtime_error("not logged in");
        }

        GitHubClient client;
        try {
            client.login_token(gh_token);
            git_thread_dispatcher.emit();
        }
        catch (const std::runtime_error &e) {
            gh_username = "";
            throw std::runtime_error("can't log in");
        }

        {
            std::lock_guard<std::mutex> lock(git_thread_mutex);
            git_thread_status = "Opening repository…";
        }
        git_thread_dispatcher.emit();

        autofree_ptr<git_repository> repo(git_repository_free);
        if (git_repository_open(&repo.ptr, notebook->remote_repo.c_str()) != 0) {
            throw std::runtime_error("error opening repo");
        }

        {
            std::lock_guard<std::mutex> lock(git_thread_mutex);
            git_thread_status = "Adding remote…";
        }
        git_thread_dispatcher.emit();
        auto my_remote = get_or_create_remote(client, repo);

        autofree_ptr<git_reference> branch(git_reference_free);
        if (const auto err = git_branch_lookup(&branch.ptr, repo, pr_update_branch.c_str(), GIT_BRANCH_LOCAL);
            err == GIT_ENOTFOUND) {
            // branch not found, fetch
            std::string refspec = "refs/heads/" + pr_update_branch;
            char *refspec_c = strdup(refspec.c_str());

            const git_strarray refs = {&refspec_c, 1};
            git_fetch_options fetch_opts = GIT_FETCH_OPTIONS_INIT;
            {
                std::lock_guard<std::mutex> lock(git_thread_mutex);
                git_thread_status = "Fetching branch…";
            }
            git_thread_dispatcher.emit();
            if (git_remote_fetch(my_remote, &refs, &fetch_opts, NULL) != 0) {
                free(refspec_c);
                throw std::runtime_error("error fetching");
            }
            free(refspec_c);

            // set up local branch
            autofree_ptr<git_annotated_commit> latest_commit(git_annotated_commit_free);
            if (git_annotated_commit_from_revspec(&latest_commit.ptr, repo,
                                                  (gh_username + "/" + pr_update_branch).c_str())
                != 0) {
                throw std::runtime_error("error getting latest commit");
            }

            autofree_ptr<git_reference> local_branch(git_reference_free);
            if (git_branch_create_from_annotated(&local_branch.ptr, repo, pr_update_branch.c_str(), latest_commit, 0)
                != 0) {
                throw std::runtime_error("error creating local branch");
            }
        }
        else if (err != 0) {
            throw std::runtime_error("branch lookup");
        }

        {
            std::lock_guard<std::mutex> lock(git_thread_mutex);
            git_thread_busy = false;
            git_thread_status = "Done";
        }
        git_thread_dispatcher.emit();
    }
    catch (const std::exception &e) {
        {
            std::lock_guard<std::mutex> lock(git_thread_mutex);
            git_thread_busy = false;
            git_thread_error = true;
            git_thread_status = "Error: " + std::string(e.what());
        }
        git_thread_dispatcher.emit();
    }
    catch (const Gio::Error &e) {
        {
            std::lock_guard<std::mutex> lock(git_thread_mutex);
            git_thread_busy = false;
            git_thread_error = true;
            git_thread_status = "IO Error: " + std::string(e.what());
        }
        git_thread_dispatcher.emit();
    }
}

} // namespace horizon
