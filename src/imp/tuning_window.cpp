#include "tuning_window.hpp"
#include "board/board.hpp"
#include "util/util.hpp"
#include <iomanip>

namespace horizon {
TuningWindow::TuningWindow(const Board *brd) : Gtk::Window(), board(brd), state_store(this, "tuning")
{
    set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
    auto header = Gtk::manage(new Gtk::HeaderBar());
    header->set_show_close_button(true);
    header->set_title("Length tuning");
    set_titlebar(*header);
    header->show();

    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    {
        {
            auto bu = Gtk::manage(new Gtk::Button("Clear"));
            bu->signal_clicked().connect([this] {
                store->clear();
                update();
            });
            bu->show();
            header->pack_start(*bu);
        }

        auto tbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4));
        tbox->property_margin() = 8;
        {
            auto la = Gtk::manage(new Gtk::Label("Velocity factor:"));
            tbox->pack_start(*la, false, false, 0);
        }

        sp_vf = Gtk::manage(new Gtk::SpinButton());
        sp_vf->set_range(22, 100);
        sp_vf->set_increments(1, 1);
        sp_vf->set_value(100);
        sp_vf->signal_value_changed().connect([this] {
            auto vf = sp_vf->get_value();
            sp_er->set_value(10000 / (vf * vf));
            update();
        });
        sp_vf->signal_output().connect([this] {
            int v = sp_vf->get_value_as_int();
            sp_vf->set_text(std::to_string(v) + " %");
            return true;
        });
        sp_vf->set_alignment(1);
        sp_vf->set_width_chars(5);
        tbox->pack_start(*sp_vf, false, false, 0);

        {
            auto la = Gtk::manage(new Gtk::Label());
            la->set_markup("ε<sub>r</sub>:");
            la->set_margin_start(4);
            la->set_tooltip_text("Dielectric constant");
            tbox->pack_start(*la, false, false, 0);
        }

        sp_er = Gtk::manage(new Gtk::SpinButton());
        sp_er->set_range(1, 20);
        sp_er->set_increments(.1, 1);
        sp_er->set_value(1);
        sp_er->set_digits(2);
        sp_er->signal_value_changed().connect([this] { sp_vf->set_value(100 / sqrt(sp_er->get_value())); });
        tbox->pack_start(*sp_er, false, false, 0);

        box->pack_start(*tbox, false, false, 0);
    }
    {
        auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
        box->pack_start(*sep, false, false, 0);
    }

    sc = Gtk::manage(new Gtk::ScrolledWindow);
    sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);

    store = Gtk::ListStore::create(list_columns);
    tree_view = Gtk::manage(new Gtk::TreeView(store));
    tree_view->get_selection()->set_mode(Gtk::SELECTION_MULTIPLE);
    tree_view->append_column("Net", list_columns.net_name);
    tree_view->get_column(0)->set_sort_column(list_columns.net_name);
    store->set_sort_column(list_columns.net_name, Gtk::SORT_ASCENDING);

    {
        auto cr = Gtk::manage(new Gtk::CellRendererToggle());
        cr->property_xalign() = 0;
        cr->signal_toggled().connect([this](const Glib::ustring &path) {
            auto it = store->get_iter(path);
            if (it) {
                Gtk::TreeModel::Row row = *it;
                row[list_columns.all_tracks] = !row[list_columns.all_tracks];
                update();
            }
        });
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("All tracks", *cr));
        auto col = tree_view->get_column(tree_view->append_column(*tvc) - 1);
        col->add_attribute(cr->property_active(), list_columns.all_tracks);
    }

    {
        auto cr = Gtk::manage(new Gtk::CellRendererToggle());
        cr->property_radio() = true;
        cr->property_xalign() = 0;
        cr->signal_toggled().connect([this](const Glib::ustring &path) {
            auto children = store->children();
            for (auto it : children) {
                Gtk::TreeModel::Row row = *it;
                row[list_columns.ref] = false;
            }
            auto it = store->get_iter(path);
            if (it) {
                Gtk::TreeModel::Row row = *it;
                row[list_columns.ref] = true;
                update();
            }
        });
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Ref", *cr));
        auto col = tree_view->get_column(tree_view->append_column(*tvc) - 1);
        col->add_attribute(cr->property_active(), list_columns.ref);
    }

    {
        auto cr = Gtk::manage(new Gtk::CellRendererProgress());
        cr->property_text_xalign() = 0;
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Length", *cr));
        tvc->set_cell_data_func(*cr, [this](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
            Gtk::TreeModel::Row row = *it;
            auto mcr = dynamic_cast<Gtk::CellRendererProgress *>(tcr);
            std::stringstream ss;
            ss.imbue(get_locale());
            ss << " " << dim_to_string(row[list_columns.length], false);
            ss << " " << std::fixed << std::setprecision(3) << std::setw(7) << std::setfill('0') << std::internal
               << row[list_columns.length_ps] << " ps";
            double delta_ps = row[list_columns.delta_ps];
            if (!isnan(delta_ps)) {
                ss << " Δ";
                if (delta_ps >= 0) {
                    ss << "+";
                }
                else {
                    ss << "−";
                }
                ss << abs(delta_ps) << " ps";
            }
            mcr->property_text() = ss.str();
            mcr->property_value() = row[list_columns.fill_value];
        });
        tree_view->append_column(*tvc);
    }


    tree_view->signal_key_press_event().connect([this](GdkEventKey *ev) {
        if (ev->keyval == GDK_KEY_Delete) {
            auto paths = tree_view->get_selection()->get_selected_rows();
            for (auto it = paths.rbegin(); it != paths.rend(); it++)
                store->erase(store->get_iter(*it));
            update();
            return true;
        }
        return false;
    });

    sc->add(*tree_view);
    {
        auto overlay = Gtk::manage(new Gtk::Overlay);
        overlay->add(*sc);
        placeholder_label = Gtk::manage(
                new Gtk::Label("No tracks to measure.\nUse \"Measure (all) tracks\" to add tracks to this list."));
        placeholder_label->set_justify(Gtk::JUSTIFY_CENTER);
        placeholder_label->set_halign(Gtk::ALIGN_CENTER);
        placeholder_label->set_valign(Gtk::ALIGN_CENTER);
        placeholder_label->get_style_context()->add_class("dim-label");
        overlay->add_overlay(*placeholder_label);
        box->pack_start(*overlay, true, true, 0);
    }

    box->show_all();
    add(*box);
}

void TuningWindow::add_tracks(const std::set<UUID> &tracks_uuid, bool all)
{
    std::map<const Net *, std::set<const Track *>> tracks;
    for (const auto &it : tracks_uuid) {
        auto track = &board->tracks.at(it);
        if (track->net) {
            tracks[track->net].insert(track);
        }
    }
    for (const auto &it : tracks) {
        Gtk::TreeModel::Row row = *store->append();
        row[list_columns.net_name] = it.first->name;
        row[list_columns.net] = it.first->uuid;
        std::set<UUID> uuids;
        for (auto track : it.second)
            uuids.insert(track->uuid);
        row[list_columns.tracks] = uuids;
        row[list_columns.all_tracks] = all;
    }
    update();
}

static int64_t get_track_length(const Track &track)
{
    return sqrt((track.from.get_position() - track.to.get_position()).mag_sq());
}

void TuningWindow::update()
{
    auto children = store->children();
    uint64_t length_max = 0;
    double c = 299.79e6 * (sp_vf->get_value() / 100);
    double ref_ps = -1;
    placeholder_label->set_visible(children.size() == 0);
    for (auto it : children) {
        Gtk::TreeModel::Row row = *it;
        uint64_t length = 0;
        if (row[list_columns.all_tracks]) {
            for (auto &it_track : board->tracks) {
                if (it_track.second.net && it_track.second.net->uuid == row[list_columns.net])
                    length += get_track_length(it_track.second);
            }
        }
        else {
            for (const auto &track_uuid : row.get_value(list_columns.tracks)) {
                if (board->tracks.count(track_uuid)) {
                    auto &track = board->tracks.at(track_uuid);
                    length += get_track_length(track);
                }
            }
        }
        row[list_columns.length] = length;
        auto length_ps = ((length / 1e9) / c) * 1e12;
        row[list_columns.length_ps] = length_ps;
        if (row[list_columns.ref])
            ref_ps = length_ps;
        length_max = std::max(length_max, length);
    }
    for (auto it : children) {
        Gtk::TreeModel::Row row = *it;
        row[list_columns.fill_value] = std::round((row[list_columns.length] * 100.0) / length_max);
        if (ref_ps >= 0) {
            row[list_columns.delta_ps] = row[list_columns.length_ps] - ref_ps;
        }
        else {
            row[list_columns.delta_ps] = NAN;
        }
    }
}

} // namespace horizon
