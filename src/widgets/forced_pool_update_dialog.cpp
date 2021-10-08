#include "forced_pool_update_dialog.hpp"
#include "pool-update/pool-update.hpp"
#include "logger/logger.hpp"

namespace horizon {

ForcedPoolUpdateDialog::ForcedPoolUpdateDialog(const std::string &bp, Gtk::Window &parent)
    : Gtk::Dialog("Pool update", parent, Gtk::DIALOG_MODAL), base_path(bp)
{
    auto hb = Gtk::manage(new Gtk::HeaderBar);
    hb->set_show_close_button(false);
    hb->set_title("Pool update");
    set_titlebar(*hb);
    hb->show_all();

    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 10));
    {
        auto box2 = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 10));
        auto la = Gtk::manage(new Gtk::Label("Updating pool for new schema"));
        box2->pack_start(*la, true, true, 0);
        spinner = Gtk::manage(new Gtk::Spinner);
        spinner->start();
        box2->pack_start(*spinner, false, false, 0);
        box->pack_start(*box2, false, false, 0);
    }
    filename_label = Gtk::manage(new Gtk::Label);
    filename_label->get_style_context()->add_class("dim-label");
    filename_label->set_ellipsize(Pango::ELLIPSIZE_START);
    filename_label->set_xalign(0);
    box->pack_start(*filename_label, false, false, 0);

    box->property_margin() = 10;
    box->show_all();
    get_content_area()->pack_start(*box, true, true, 0);

    thread = std::thread(&ForcedPoolUpdateDialog::pool_update_thread, this);

    dispatcher.connect([this] {
        decltype(pool_update_status_queue) my_queue;
        {
            std::lock_guard<std::mutex> guard(pool_update_status_queue_mutex);
            my_queue.splice(my_queue.begin(), pool_update_status_queue);
        }

        if (my_queue.size()) {
            const auto last_info = pool_update_last_info;
            for (const auto &[last_status, last_filename, last_msg] : my_queue) {
                if (last_status == PoolUpdateStatus::DONE) {
                    spinner->stop();
                    // delay prevents deleted dispatcher warnings
                    Glib::signal_timeout().connect_once([this] { response(1); }, 200);
                }
                else if (last_status == PoolUpdateStatus::INFO) {
                    pool_update_last_info = last_msg;
                }
                else if (last_status == PoolUpdateStatus::FILE_ERROR) {
                    Logger::log_warning(last_msg, Logger::Domain::POOL_UPDATE, last_filename);
                }
            }
            if (pool_update_last_info != last_info)
                filename_label->set_text(pool_update_last_info);
        }
    });
}

void ForcedPoolUpdateDialog::pool_update_thread()
{
    try {
        pool_update(
                base_path,
                [this](PoolUpdateStatus st, std::string filename, std::string msg) {
                    {
                        std::lock_guard<std::mutex> guard(pool_update_status_queue_mutex);
                        pool_update_status_queue.emplace_back(st, filename, msg);
                    }
                    dispatcher.emit();
                },
                true);
        return;
    }
    catch (const std::exception &e) {
        Logger::log_critical("caught exception", Logger::Domain::POOL_UPDATE, e.what());
    }
    catch (const Glib::FileError &e) {
        Logger::log_critical("caught file exception", Logger::Domain::POOL_UPDATE, e.what());
    }
    catch (const Glib::Error &e) {
        Logger::log_critical("caught GLib exception", Logger::Domain::POOL_UPDATE, e.what());
    }
    catch (...) {
        Logger::log_critical("caught unknown exception", Logger::Domain::POOL_UPDATE);
    }
    {
        std::lock_guard<std::mutex> guard(pool_update_status_queue_mutex);
        pool_update_status_queue.emplace_back(PoolUpdateStatus::DONE, "", "Error");
    }
    dispatcher.emit();
}

ForcedPoolUpdateDialog::~ForcedPoolUpdateDialog()
{
    thread.join();
}

} // namespace horizon
