#include "pool_remote_box.hpp"
#include <git2.h>
#include <git2/cred_helpers.h>
#include <git2/sys/repository.h>
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

namespace horizon {

class SignInDialog : public Gtk::Dialog {
public:
    SignInDialog(Gtk::Window *parent);

    Gtk::Entry *user_entry = nullptr;
    Gtk::Entry *password_entry = nullptr;
};

SignInDialog::SignInDialog(Gtk::Window *parent)
    : Gtk::Dialog("Sign in with GitHub", *parent,
                  Gtk::DialogFlags::DIALOG_MODAL | Gtk::DialogFlags::DIALOG_USE_HEADER_BAR)
{
    add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
    add_button("Sign in", Gtk::ResponseType::RESPONSE_OK);
    set_default_response(Gtk::RESPONSE_OK);
    auto grid = Gtk::manage(new Gtk::Grid);
    grid->set_row_spacing(10);
    grid->set_column_spacing(10);
    grid->set_halign(Gtk::ALIGN_CENTER);

    int top = 0;
    user_entry = Gtk::manage(new Gtk::Entry);
    grid_attach_label_and_widget(grid, "Username", user_entry, top);

    password_entry = Gtk::manage(new Gtk::Entry);
    password_entry->set_visibility(false);
    grid_attach_label_and_widget(grid, "Password", password_entry, top);

    user_entry->signal_activate().connect([this] { password_entry->grab_focus(); });
    password_entry->signal_activate().connect([this] { response(Gtk::RESPONSE_OK); });

    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 20));
    box->property_margin() = 20;
    box->pack_start(*grid, true, true, 0);

    auto la = Gtk::manage(new Gtk::Label());
    la->set_markup(
            "Your credentials will not be saved.\nDon't have a GitHub account? "
            "<a href='https://github.com/join'>Sign up</a>");
    la->get_style_context()->add_class("dim-label");
    box->pack_start(*la, false, false, 0);

    get_content_area()->add(*box);
    box->show_all();
}

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
    x->get_widget("button_refresh_pull_requests", refresh_prs_button);
    x->get_widget("remote_gh_signed_in_label", gh_signed_in_label);
    x->get_widget("remote_pr_title_entry", pr_title_entry);
    x->get_widget("remote_pr_body_textview", pr_body_textview);
    x->get_widget("pr_spinner", pr_spinner);

    pull_requests_listbox->set_header_func(sigc::ptr_fun(header_func_separator));

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
            gh_signed_in_label->set_text(gh_username);
        }
        else {
            gh_signed_in_label->set_text("not signed in");
        }
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
        if (git_thread_mode == GitThreadMode::PULL_REQUEST && !git_thread_busy && !git_thread_error) {
            items_merge.clear();
            models_merge.clear();
            update_items_merge();
            pr_title_entry->set_text("");
            pr_body_textview->get_buffer()->set_text("");
            handle_refresh_prs();
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

    merge_items_view->append_column("Name", list_columns.name);

    merge_items_clear_button->signal_clicked().connect([this] {
        items_merge.clear();
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


    update_items_merge();
}

class PullRequestItemBox : public Gtk::Box {
public:
    PullRequestItemBox(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, const json &j);
    static PullRequestItemBox *create(const json &j);
    const std::string filename;

private:
};

PullRequestItemBox::PullRequestItemBox(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, const json &j)
    : Gtk::Box(cobject)
{
    Gtk::Label *name_label, *descr_label, *number_label;
    Gtk::LinkButton *link_button;
    x->get_widget("pull_request_item_name_label", name_label);
    x->get_widget("pull_request_item_descr_label", descr_label);
    x->get_widget("pull_request_item_number_label", number_label);
    x->get_widget("pull_request_item_link", link_button);

    name_label->set_text(j.at("title").get<std::string>());
    int nr = j.at("number");
    number_label->set_text("#" + std::to_string(nr));
    link_button->set_label("Open on GitHub");
    link_button->set_uri(j.at("_links").at("html").at("href").get<std::string>());
    std::string open_user = j.at("user").at("login").get<std::string>();
    std::string created_at = j.at("created_at").get<std::string>();
    descr_label->set_text("opened by " + open_user + " on " + created_at);
}

PullRequestItemBox *PullRequestItemBox::create(const json &j)
{
    PullRequestItemBox *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/net/carrotIndustries/horizon/pool-prj-mgr/window.ui", "pull_request_item");
    x->get_widget_derived("pull_request_item", w, j);
    w->reference();
    return w;
}

void PoolRemoteBox::update_prs()
{
    {
        auto children = pull_requests_listbox->get_children();
        for (auto it : children) {
            delete it;
        }
    }

    for (auto it = pull_requests.cbegin(); it != pull_requests.cend(); ++it) {
        auto box = PullRequestItemBox::create(it.value());
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

std::set<std::pair<ObjectType, UUID>> PoolRemoteBox::get_referenced(ObjectType ty, const UUID &uu)
{
    std::set<std::pair<ObjectType, UUID>> items;
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
    x->add_from_resource("/net/carrotIndustries/horizon/pool-prj-mgr/window.ui", widgets);
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

    pr_status_dispatcher.reset("Starting...");
    std::thread thr(&PoolRemoteBox::refresh_prs_thread, this);

    thr.detach();
}

void PoolRemoteBox::handle_create_pr()
{
    if (notebook->pool_updating)
        return;

    auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));
    if (gh_username.size() == 0) {
        SignInDialog dia_sign_in(top);
        if (dia_sign_in.run() == Gtk::RESPONSE_OK) {
            gh_username = dia_sign_in.user_entry->get_text();
            gh_password = dia_sign_in.password_entry->get_text();
        }
        else {
            return;
        }
    }

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

void PoolRemoteBox::update_items_merge()
{
    item_store->clear();
    auto can_merge = items_merge.size() > 0 || models_merge.size() > 0;
    create_pr_button->set_sensitive(can_merge);
    merge_items_clear_button->set_sensitive(can_merge);
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
        }
    }

    for (const auto &it : models_merge) {
        Gtk::TreeModel::Row row = *item_store->append();
        row[list_columns.name] = Glib::path_get_basename(it);
        row[list_columns.type] = ObjectType::MODEL_3D;
        row[list_columns.filename] = it;
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
            git_thread_status = "Opening repository...";
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
            git_thread_status = "Fetching...";
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
            git_thread_status = "Updating remote pool...";
        }
        git_thread_dispatcher.emit();
        pool_update(notebook->remote_repo);

        {
            std::lock_guard<std::mutex> lock(git_thread_mutex);
            git_thread_status = "Updating local pool...";
        }
        git_thread_dispatcher.emit();
        pool_update(notebook->base_path);

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

void PoolRemoteBox::create_pr_thread()
{
    try {

        {
            std::lock_guard<std::mutex> lock(git_thread_mutex);
            git_thread_status = "Authenticating...";
        }
        git_thread_dispatcher.emit();

        GitHubClient client;
        json user_info;
        try {
            user_info = client.login(gh_username, gh_password);
            gh_username = user_info.at("login").get<std::string>();
            git_thread_dispatcher.emit();
        }
        catch (const std::runtime_error &e) {
            gh_username = "";
            throw std::runtime_error("can't sign in");
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
            git_thread_status = "Opening repository...";
        }
        git_thread_dispatcher.emit();

        autofree_ptr<git_repository> repo(git_repository_free);
        if (git_repository_open(&repo.ptr, notebook->remote_repo.c_str()) != 0) {
            throw std::runtime_error("error opening repo");
        }


        std::string user_email;
        std::string user_name;
        std::cout << std::setw(4) << user_info << std::endl;
        try {
            user_email = user_info.at("email").get<std::string>();
        }
        catch (...) {
            user_email = "horizon@0x83.eu";
        }

        try {
            user_name = user_info.at("name").get<std::string>();
        }
        catch (...) {
            user_name = "horizon pool manager";
        }


        autofree_ptr<git_remote> my_remote(git_remote_free);
        if (git_remote_lookup(&my_remote.ptr, repo, gh_username.c_str()) != 0) {
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
                        git_thread_status = "Waiting for fork... (" + std::string(e.what()) + ")";
                    }
                    git_thread_dispatcher.emit();
                    Glib::usleep(1e6);
                }
                catch (...) {
                    retries--;
                    {
                        std::lock_guard<std::mutex> lock(git_thread_mutex);
                        git_thread_status = "Waiting for fork...";
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
        }

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

        git_oid tree_oid;
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

            if (git_index_write_tree(&tree_oid, index) != 0) {
                throw std::runtime_error("error saving index");
            }
        }

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
            git_signature_now(&signature.ptr, user_name.c_str(), user_email.c_str());

            git_oid new_commit_oid;
            if (git_commit_create(&new_commit_oid, repo, "HEAD", signature, signature, "UTF-8", pr_title.c_str(), tree,
                                  1, &parent_commit_p)
                != 0) {
                throw std::runtime_error("error committing");
            }
        }

        {
            std::lock_guard<std::mutex> lock(git_thread_mutex);
            git_thread_status = "Pushing...";
        }
        git_thread_dispatcher.emit();

        checkout_master(repo);

        { // push
            git_push_options push_opts = GIT_PUSH_OPTIONS_INIT;
            push_opts.callbacks.credentials = git_cred_userpass;
            git_cred_userpass_payload payload;
            payload.username = gh_username.c_str();
            payload.password = gh_password.c_str();
            push_opts.callbacks.payload = &payload;

            std::string refspec = "refs/heads/" + new_branch_name;
            char *refspec_c = strdup(refspec.c_str());

            const git_strarray refs = {&refspec_c, 1};

            if (git_remote_push(my_remote, &refs, &push_opts) != 0) {
                auto err = giterr_last();
                std::string errstr = err->message;
                free(refspec_c);
                throw std::runtime_error("error pushing " + errstr);
            }
            free(refspec_c);
        }

        {
            std::lock_guard<std::mutex> lock(git_thread_mutex);
            git_thread_status = "Creating pull request...";
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
} // namespace horizon
