#include "plane_update.hpp"
#include "board/board.hpp"
#include "logger/logger.hpp"
#include "util/util.hpp"
#include "util/gtk_util.hpp"

namespace horizon {

PlaneUpdateDialog::PlaneUpdateDialog(Gtk::Window &parent, Board &brd, Plane *plane)
    : Gtk::Dialog("Plane update", parent, Gtk::DIALOG_MODAL)
{
    auto hb = Gtk::manage(new Gtk::HeaderBar);
    hb->set_show_close_button(false);
    hb->set_title("Plane update");
    set_titlebar(*hb);
    hb->show_all();
    auto cancel_button = Gtk::manage(new Gtk::Button("Cancel"));
    cancel_button->show();
    hb->pack_end(*cancel_button);
    cancel_button->signal_clicked().connect([this] {
        status_label->set_text("Cancelling…");
        cancel = true;
    });
    if (!plane)
        set_default_size(450, 300);
    else
        set_default_size(300, -1);

    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    {
        auto box2 = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 10));
        box2->property_margin() = 10;
        if (plane)
            status_label = Gtk::manage(new Gtk::Label("Starting…"));
        else
            status_label = Gtk::manage(new Gtk::Label("Updating planes…"));
        label_set_tnum(status_label);
        status_label->set_xalign(0);
        box2->pack_start(*status_label, true, true, 0);
        spinner = Gtk::manage(new Gtk::Spinner);
        spinner->start();
        box2->pack_start(*spinner, false, false, 0);
        box->pack_start(*box2, false, false, 0);
    }
    if (!plane) {
        store = Gtk::ListStore::create(list_columns);

        {
            std::vector<const Plane *> planes;
            planes.reserve(brd.planes.size());
            for (const auto &[uu, it] : brd.planes) {
                planes.push_back(&it);
            }
            std::sort(planes.begin(), planes.end(), [](auto a, auto b) { return a->priority < b->priority; });
            for (auto it : planes) {
                plane_status.emplace(it->uuid, "Waiting for other planes…");
                Gtk::TreeModel::Row row = *store->append();
                row[list_columns.layer] = brd.get_layers().at(it->polygon->layer).name;
                row[list_columns.net] = brd.block->get_net_name(it->net->uuid);
                row[list_columns.status] = "Waiting for other planes…";
                row[list_columns.plane] = it->uuid;
            }
        }

        view = Gtk::manage(new Gtk::TreeView(store));
        view->get_selection()->set_mode(Gtk::SELECTION_NONE);
        view->append_column("Layer", list_columns.layer);
        view->append_column("Net", list_columns.net);
        view->append_column("Status", list_columns.status);

        auto sc = Gtk::manage(new Gtk::ScrolledWindow());
        sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
        sc->add(*view);
        sc->show_all();
        {
            auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
            box->pack_start(*sep, false, false, 0);
        }
        box->pack_start(*sc, true, true, 0);
    }
    else {
        plane_status.emplace(plane->uuid, "Starting…");
    }

    box->show_all();
    get_content_area()->pack_start(*box, true, true, 0);
    get_content_area()->set_border_width(0);

    dispatcher.connect([this] {
        if (done) {
            spinner->stop();
            // delay prevents deleted dispatcher warnings
            Glib::signal_timeout().connect_once(sigc::track_obj([this] { response(1); }, *this), 200);
        }
        else {
            if (store) {
                size_t n_done = 0;
                for (auto it : store->children()) {
                    Gtk::TreeModel::Row row = *it;
                    const auto &st = plane_status.at(row[list_columns.plane]);
                    row[list_columns.status] = st;
                    if (st == "Done")
                        n_done++;
                }
                if (!cancel)
                    status_label->set_text("Updating planes… " + format_m_of_n(n_done, plane_status.size()) + " done");
            }
            else {
                if (!cancel)
                    status_label->set_text(plane_status.begin()->second);
            }
        }
    });

    thread = std::thread(&PlaneUpdateDialog::plane_update_thread, this, std::ref(brd), plane);
}

void PlaneUpdateDialog::plane_update_thread(Board &brd, Plane *plane)
{
    try {
        auto cb = [this](const Plane &p, const std::string &s) {
            {
                std::lock_guard<std::mutex> guard(mutex);
                plane_status.at(p.uuid) = s;
            }
            dispatcher.emit();
        };
        if (plane) {
            brd.update_plane(plane, nullptr, nullptr, cb, cancel);
        }
        else {
            brd.update_planes(cb, cancel);
        }
    }
    catch (const std::exception &e) {
        Logger::log_critical("caught exception in plane update", Logger::Domain::BOARD, e.what());
    }
    catch (...) {
        Logger::log_critical("caught unknown exception in plane update", Logger::Domain::BOARD);
    }
    done = true;
    dispatcher.emit();
}

PlaneUpdateDialog::~PlaneUpdateDialog()
{
    thread.join();
}

} // namespace horizon
