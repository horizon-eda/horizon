#pragma once
#include <gtkmm.h>
#include <git2.h>
#include "util/status_dispatcher.hpp"

namespace horizon {

class PoolDownloadWindow : public Gtk::Window {
public:
    PoolDownloadWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, const class PoolIndex *idx);
    static PoolDownloadWindow *create(const class PoolIndex *idx);

    typedef sigc::signal<void> type_signal_downloaded;
    type_signal_downloaded signal_downloaded()
    {
        return s_signal_downloaded;
    }

private:
    static int git_transfer_cb(const git_transfer_progress *stats, void *payload);
    static void git_checkout_progress_cb(const char *path, size_t completed_steps, size_t total_steps, void *payload);
    bool downloading = false;
    bool download_cancel = false;

    void handle_do_download();
    void handle_cancel();

    void download_thread(std::string gh_username, std::string gh_repo, std::string dest_dir);

    Gtk::Button *download_button = nullptr;
    Gtk::Button *cancel_button = nullptr;

    StatusDispatcher download_status_dispatcher;

    Gtk::Revealer *download_revealer = nullptr;
    Gtk::Label *download_label = nullptr;
    Gtk::Spinner *download_spinner = nullptr;
    Gtk::ProgressBar *download_progress = nullptr;

    Gtk::Entry *download_gh_username_entry = nullptr;
    Gtk::Entry *download_gh_repo_entry = nullptr;
    Gtk::Entry *download_dest_dir_entry = nullptr;
    Gtk::Button *download_dest_dir_button = nullptr;

    type_signal_downloaded s_signal_downloaded;
};
} // namespace horizon
