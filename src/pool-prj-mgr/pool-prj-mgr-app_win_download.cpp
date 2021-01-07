#include "pool-prj-mgr-app_win.hpp"
#include "util/github_client.hpp"
#include "pool-update/pool-update.hpp"
#include <thread>
#include "pool/pool.hpp"
#include "util/util.hpp"
#include "util/autofree_ptr.hpp"

namespace horizon {

void PoolProjectManagerAppWindow::handle_do_download()
{

    std::string dest_dir = download_dest_dir_entry->get_text();
    if (dest_dir.size()) {
        download_status_dispatcher.reset("Starting…");
        download_cancel = false;
        std::thread dl_thread(&PoolProjectManagerAppWindow::download_thread, this,
                              download_gh_username_entry->get_text(), download_gh_repo_entry->get_text(), dest_dir);
        dl_thread.detach();
        downloading = true;
    }
    else {
        Gtk::MessageDialog md(*this, "Destination directory needs to be set", false /* use_markup */,
                              Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
        md.run();
    }
}

int PoolProjectManagerAppWindow::git_transfer_cb(const git_transfer_progress *stats, void *payload)
{
    auto self = static_cast<PoolProjectManagerAppWindow *>(payload);
    const std::string msg = "Cloning: fetching object " + format_m_of_n(stats->received_objects, stats->total_objects);
    self->download_status_dispatcher.set_status(StatusDispatcher::Status::BUSY, msg,
                                                ((double)stats->received_objects) / stats->total_objects);
    if (self->download_cancel)
        return -1;
    return 0;
}

void PoolProjectManagerAppWindow::git_checkout_progress_cb(const char *path, size_t completed_steps, size_t total_steps,
                                                           void *payload)
{
    auto self = static_cast<PoolProjectManagerAppWindow *>(payload);
    const std::string msg = "Cloning: checking out " + format_m_of_n(completed_steps, total_steps);
    self->download_status_dispatcher.set_status(StatusDispatcher::Status::BUSY, msg,
                                                ((double)completed_steps) / total_steps);
}

#define RETURN_IF_CANCELLED                                                                                            \
    if (download_cancel) {                                                                                             \
        download_status_dispatcher.set_status(StatusDispatcher::Status::BUSY, "Cancelled, cleaning up");               \
        rmdir_recursive(dest_dir);                                                                                     \
        download_status_dispatcher.set_status(StatusDispatcher::Status::DONE, "Cancelled");                            \
        return;                                                                                                        \
    }

void PoolProjectManagerAppWindow::download_thread(std::string gh_username, std::string gh_repo, std::string dest_dir)
{
    try {

        if (!Glib::file_test(dest_dir, Glib::FILE_TEST_IS_DIR)) {
            Gio::File::create_for_path(dest_dir)->make_directory_with_parents();
        }

        {
            Glib::Dir dd(dest_dir);
            for (const auto &it : dd) {
                (void)sizeof it;
                throw std::runtime_error("destination dir is not empty");
            }
        }

        download_status_dispatcher.set_status(StatusDispatcher::Status::BUSY, "Fetching clone URL…");

        GitHubClient client;
        json repo = client.get_repo(gh_username, gh_repo);
        std::string clone_url = repo.at("clone_url");

        autofree_ptr<git_repository> cloned_repo(git_repository_free);
        git_clone_options clone_opts = GIT_CLONE_OPTIONS_INIT;
        git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;

        checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE;
        clone_opts.checkout_opts = checkout_opts;
        clone_opts.fetch_opts.callbacks.transfer_progress = &PoolProjectManagerAppWindow::git_transfer_cb;
        clone_opts.fetch_opts.callbacks.payload = this;
        clone_opts.checkout_opts.progress_cb = &PoolProjectManagerAppWindow::git_checkout_progress_cb;
        clone_opts.checkout_opts.progress_payload = this;

        download_status_dispatcher.set_status(StatusDispatcher::Status::BUSY, "Cloning repository…");

        auto remote_dir = Glib::build_filename(dest_dir, ".remote");
        Gio::File::create_for_path(remote_dir)->make_directory_with_parents();
        int error = git_clone(&cloned_repo.ptr, clone_url.c_str(), remote_dir.c_str(), &clone_opts);

        if (error != 0) {
            RETURN_IF_CANCELLED
            auto gerr = giterr_last();
            throw std::runtime_error("git error " + std::to_string(gerr->klass) + " " + std::string(gerr->message));
        }
        RETURN_IF_CANCELLED


        download_status_dispatcher.set_status(StatusDispatcher::Status::BUSY, "Updating remote pool…");

        pool_update(remote_dir);
        RETURN_IF_CANCELLED
        Pool pool_remote(remote_dir);

        download_status_dispatcher.set_status(StatusDispatcher::Status::BUSY, "Copying…");

        size_t n_total = 0;
        size_t i = 0;
        {
            SQLite::Query q_count(pool_remote.db,
                                  "SELECT SUM(n) FROM (SELECT COUNT(filename) AS n FROM all_items_view UNION "
                                  "SELECT COUNT(DISTINCT model_filename) AS n FROM models);");
            q_count.step();
            n_total = q_count.get<int>(0);
        }
        {
            SQLite::Query q(pool_remote.db, "SELECT filename FROM all_items_view");
            while (q.step()) {
                RETURN_IF_CANCELLED
                const std::string msg = "Copying " + format_m_of_n(i, n_total);
                download_status_dispatcher.set_status(StatusDispatcher::Status::BUSY, msg, ((double)i) / n_total);
                i++;
                std::string filename = q.get<std::string>(0);
                auto dirname = Glib::build_filename(dest_dir, Glib::path_get_dirname(filename));
                if (!Glib::file_test(dirname, Glib::FILE_TEST_IS_DIR)) {
                    Gio::File::create_for_path(dirname)->make_directory_with_parents();
                }
                Gio::File::create_for_path(Glib::build_filename(remote_dir, filename))
                        ->copy(Gio::File::create_for_path(Glib::build_filename(dest_dir, filename)));
            }
        }
        {
            SQLite::Query q(pool_remote.db, "SELECT DISTINCT model_filename FROM models");
            while (q.step()) {
                RETURN_IF_CANCELLED
                const std::string msg = "Copying " + format_m_of_n(i, n_total);
                download_status_dispatcher.set_status(StatusDispatcher::Status::BUSY, msg, ((double)i) / n_total);
                i++;
                std::string filename = q.get<std::string>(0);
                auto remote_filename = Glib::build_filename(remote_dir, filename);
                if (Glib::file_test(remote_filename, Glib::FILE_TEST_IS_REGULAR)) {
                    auto dirname = Glib::build_filename(dest_dir, Glib::path_get_dirname(filename));
                    if (!Glib::file_test(dirname, Glib::FILE_TEST_IS_DIR)) {
                        Gio::File::create_for_path(dirname)->make_directory_with_parents();
                    }
                    Gio::File::create_for_path(remote_filename)
                            ->copy(Gio::File::create_for_path(Glib::build_filename(dest_dir, filename)));
                }
            }
        }

        Gio::File::create_for_path(Glib::build_filename(remote_dir, "pool.json"))
                ->copy(Gio::File::create_for_path(Glib::build_filename(dest_dir, "pool.json")));

        {
            auto tables_json = Glib::build_filename(remote_dir, "tables.json");
            if (Glib::file_test(tables_json, Glib::FILE_TEST_IS_REGULAR)) {
                Gio::File::create_for_path(Glib::build_filename(remote_dir, "tables.json"))
                        ->copy(Gio::File::create_for_path(Glib::build_filename(dest_dir, "tables.json")));
            }
        }
        {
            auto help_src = Glib::build_filename(remote_dir, "layer_help");
            if (Glib::file_test(help_src, Glib::FILE_TEST_IS_DIR)) {
                auto help_dest = Glib::build_filename(dest_dir, "layer_help");
                Gio::File::create_for_path(help_dest)->make_directory_with_parents();
                Glib::Dir dir(help_src);
                for (const auto &it : dir) {
                    Gio::File::create_for_path(Glib::build_filename(help_src, it))
                            ->copy(Gio::File::create_for_path(Glib::build_filename(help_dest, it)));
                }
            }
        }
        RETURN_IF_CANCELLED
        download_status_dispatcher.set_status(StatusDispatcher::Status::DONE, "Done");
    }
    catch (const std::exception &e) {
        download_status_dispatcher.set_status(StatusDispatcher::Status::ERROR, "Error: " + std::string(e.what()));
    }
    catch (const Gio::Error &e) {
        download_status_dispatcher.set_status(StatusDispatcher::Status::ERROR, "Error: " + std::string(e.what()));
    }
}

} // namespace horizon
