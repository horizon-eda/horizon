#include "airwire_filter_window.hpp"
#include "board/board.hpp"
#include "util/util.hpp"
#include "util/gtk_util.hpp"
#include "util/str_util.hpp"
#include "widgets/cell_renderer_color_box.hpp"
#include "nlohmann/json.hpp"

namespace horizon {
AirwireFilterWindow *AirwireFilterWindow::create(Gtk::Window *p, const class Board &b)
{
    AirwireFilterWindow *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/horizon-eda/horizon/imp/airwire_filter.ui");
    x->get_widget_derived("window", w, b);
    w->set_transient_for(*p);
    return w;
}

AirwireFilterWindow::AirwireFilterWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x,
                                         const class Board &bd)
    : Gtk::Window(cobject), brd(bd), block(*bd.block)
{
    install_esc_to_close(*this);

    GET_WIDGET(treeview);
    GET_WIDGET(all_on_button);
    GET_WIDGET(all_off_button);
    GET_WIDGET(search_button);
    GET_WIDGET(search_revealer);
    GET_WIDGET(airwires_button);
    GET_WIDGET(airwires_revealer);
    GET_WIDGET(netclass_combo);
    GET_WIDGET(search_entry);
    GET_WIDGET(airwires_only_cb);

    airwires_button->signal_toggled().connect(
            [this] { airwires_revealer->set_reveal_child(airwires_button->get_active()); });

    search_button->signal_toggled().connect([this] {
        search_revealer->set_reveal_child(search_button->get_active());
        if (search_button->get_active())
            search_entry->grab_focus();
    });

    store = Gtk::ListStore::create(list_columns);
    store_filtered = Gtk::TreeModelFilter::create(store);
    store_filtered->set_visible_func([this](const Gtk::TreeModel::const_iterator &it) -> bool {
        const Gtk::TreeModel::Row row = *it;
        if (airwires_only_cb->get_active()) {
            if (row[list_columns.n_airwires] == 0)
                return false;
        }
        if (netclass_filter) {
            if (row[list_columns.net_class] != netclass_filter)
                return false;
        }
        if (search_spec.has_value()) {
            return search_spec->match(row.get_value(list_columns.net_name).casefold());
        }
        return true;
    });
    store_sorted = Gtk::TreeModelSort::create(store_filtered);

    treeview->set_model(store_sorted);
    {
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Net"));

        auto cr_color = Gtk::manage(new CellRendererColorBox);
        tvc->pack_start(*cr_color, false);
        tvc->add_attribute(cr_color->property_color(), list_columns.color);

        auto cr = Gtk::manage(new Gtk::CellRendererText());
        tvc->pack_start(*cr, true);
        tvc->add_attribute(cr->property_text(), list_columns.net_name);
        tvc->set_sort_column(list_columns.net_name);
        treeview->append_column(*tvc);
    }
    {
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Net class"));
        auto cr = Gtk::manage(new Gtk::CellRendererText());
        tvc->pack_start(*cr, true);
        tvc->add_attribute(cr->property_text(), list_columns.net_class_name);
        tvc->set_sort_column(list_columns.net_class_name);
        treeview->append_column(*tvc);
    }
    {
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Airwires"));
        auto cr_toggle = Gtk::manage(new Gtk::CellRendererToggle());
        cr_toggle->property_xalign() = 1;
        tvc->pack_start(*cr_toggle, false);
        tvc->add_attribute(cr_toggle->property_active(), list_columns.airwires_visible);
        cr_toggle->signal_toggled().connect([this](const Glib::ustring &path) {
            auto it = store->get_iter(store_filtered->convert_path_to_child_path(
                    store_sorted->convert_path_to_child_path(Gtk::TreeModel::Path(path))));
            if (it) {
                Gtk::TreeModel::Row row = *it;
                const bool v = !row[list_columns.airwires_visible];
                airwires_visible[row[list_columns.net]] = v;
                row[list_columns.airwires_visible] = v;
                s_signal_changed.emit();
            }
        });


        auto cr = Gtk::manage(new Gtk::CellRendererText());
        tvc->pack_start(*cr, true);
        tvc->add_attribute(cr->property_text(), list_columns.n_airwires);
        tvc->set_sort_column(list_columns.n_airwires);

        treeview->append_column(*tvc);
    }
    store_sorted->set_sort_column(list_columns.net_name, Gtk::SORT_ASCENDING);
    store_sorted->set_sort_column(list_columns.n_airwires, Gtk::SORT_ASCENDING);
    store_sorted->set_sort_func(list_columns.net_name,
                                [this](const Gtk::TreeModel::iterator &ia, const Gtk::TreeModel::iterator &ib) {
                                    Gtk::TreeModel::Row ra = *ia;
                                    Gtk::TreeModel::Row rb = *ib;
                                    Glib::ustring a = ra[list_columns.net_name];
                                    Glib::ustring b = rb[list_columns.net_name];
                                    return strcmp_natural(a, b);
                                });

    all_on_button->signal_clicked().connect([this] { set_all(true); });
    all_off_button->signal_clicked().connect([this] { set_all(false); });


    append_context_menu_item("Check", MenuOP::CHECK);
    append_context_menu_item("Uncheck", MenuOP::UNCHECK);
    append_context_menu_item("Toggle", MenuOP::TOGGLE);
    append_context_menu_item("Set color", MenuOP::SET_COLOR);
    append_context_menu_item("Clear color", MenuOP::CLEAR_COLOR);

    treeview->signal_button_press_event().connect(
            [this](GdkEventButton *ev) {
                Gtk::TreeModel::Path path;
                if (gdk_event_triggers_context_menu((GdkEvent *)ev)
                    && treeview->get_selection()->count_selected_rows()) {
#if GTK_CHECK_VERSION(3, 22, 0)
                    context_menu.popup_at_pointer((GdkEvent *)ev);
#else
                    context_menu.popup(ev->button, gtk_get_current_event_time());
#endif
                    return true;
                }
                return false;
            },
            false);

    treeview->get_selection()->signal_changed().connect([this] {
        std::set<UUID> nets;
        auto rows = treeview->get_selection()->get_selected_rows();
        for (const auto &path : rows) {
            Gtk::TreeModel::Row row = *store_sorted->get_iter(path);
            nets.emplace(row[list_columns.net]);
        }
        s_signal_selection_changed.emit(nets);
    });

    netclass_combo->signal_changed().connect([this] {
        try {
            netclass_filter = UUID(netclass_combo->get_active_id());
        }
        catch (...) {
            netclass_filter = UUID();
        }
        store_filtered->refilter();
    });
    search_entry->signal_search_changed().connect([this] {
        search_spec.reset();
        auto utxt = search_entry->get_text();
        std::string txt = utxt.casefold();
        trim(txt);
        if (txt.size())
            search_spec.emplace("*" + txt + "*");
        store_filtered->refilter();
    });
    airwires_only_cb->signal_toggled().connect([this] { store_filtered->refilter(); });
    s_signal_changed.connect([this] {
        bool all_on = true;
        bool all_off = true;
        for (const auto &it : store->children()) {
            Gtk::TreeModel::Row row = *it;
            const bool v = row[list_columns.airwires_visible];
            if (v)
                all_off = false;
            else
                all_on = false;
        }
        all_on_button->set_sensitive(!all_on);
        all_off_button->set_sensitive(!all_off);
    });
}

bool AirwireFilterWindow::airwire_is_visible(const UUID &net) const
{
    if (airwires_visible.count(net))
        return airwires_visible.at(net);
    else
        return true;
}

void AirwireFilterWindow::set_all(bool v)
{
    for (auto &it : store->children()) {
        Gtk::TreeModel::Row row = *it;
        row[list_columns.airwires_visible] = v;
        airwires_visible[row[list_columns.net]] = v;
    }
    s_signal_changed.emit();
}

void AirwireFilterWindow::set_only(const std::set<UUID> &nets)
{
    for (auto &it : store->children()) {
        Gtk::TreeModel::Row row = *it;
        const bool v = nets.count(row[list_columns.net]);
        row[list_columns.airwires_visible] = v;
        airwires_visible[row[list_columns.net]] = v;
    }
    s_signal_changed.emit();
}

static Gdk::RGBA rgba_from_colori(ColorI c)
{
    Gdk::RGBA rg;
    rg.set_red(c.r / 255.);
    rg.set_green(c.g / 255.);
    rg.set_blue(c.b / 255.);
    rg.set_alpha(1);
    return rg;
}

void AirwireFilterWindow::append_context_menu_item(const std::string &name, MenuOP op)
{
    auto it = Gtk::manage(new Gtk::MenuItem(name));
    it->signal_activate().connect([this, op] {
        auto rows = treeview->get_selection()->get_selected_rows();
        ColorI color;
        Gdk::RGBA rgba;
        if (op == MenuOP::SET_COLOR) {
            auto cdia = GTK_COLOR_CHOOSER_DIALOG(gtk_color_chooser_dialog_new("Pick net color", gobj()));
            auto dia = Glib::wrap(cdia, false);
            dia->property_show_editor() = true;
            dia->set_use_alpha(false);
            for (const auto &path : rows) {
                Gtk::TreeModel::Row row = *store->get_iter(
                        store_filtered->convert_path_to_child_path(store_sorted->convert_path_to_child_path(path)));
                UUID net = row[list_columns.net];
                if (net_colors.count(net)) {
                    const auto c = net_colors.at(net);
                    dia->set_rgba(rgba_from_colori(c));
                    break;
                }
            }


            if (dia->run() == Gtk::RESPONSE_OK) {
                rgba = dia->get_rgba();
                color.r = rgba.get_red() * 255;
                color.g = rgba.get_green() * 255;
                color.b = rgba.get_blue() * 255;
                gtk_widget_destroy(GTK_WIDGET(cdia));
            }
            else {
                gtk_widget_destroy(GTK_WIDGET(cdia));
                return;
            }
        }
        for (const auto &path : rows) {
            Gtk::TreeModel::Row row = *store->get_iter(
                    store_filtered->convert_path_to_child_path(store_sorted->convert_path_to_child_path(path)));
            switch (op) {
            case MenuOP::CHECK:
                row[list_columns.airwires_visible] = true;
                airwires_visible[row[list_columns.net]] = true;
                break;
            case MenuOP::UNCHECK:
                row[list_columns.airwires_visible] = false;
                airwires_visible[row[list_columns.net]] = false;
                break;
            case MenuOP::TOGGLE:
                row[list_columns.airwires_visible] = !row[list_columns.airwires_visible];
                airwires_visible[row[list_columns.net]] = row[list_columns.airwires_visible];
                break;
            case MenuOP::CLEAR_COLOR: {
                auto v = row.get_value(list_columns.color);
                if (v.gobj()) {
                    v.set_alpha(0);
                    row.set_value(list_columns.color, v);
                }
                net_colors.erase(row[list_columns.net]);
            } break;
            case MenuOP::SET_COLOR:
                row[list_columns.color] = rgba;
                net_colors[row[list_columns.net]] = color;
                break;
            }
        }
        s_signal_changed.emit();
    });
    context_menu.append(*it);
    it->show();
}

void AirwireFilterWindow::update_nets()
{
    store->clear();
    for (const auto &[net_uuid, net] : block.nets) {
        Gtk::TreeModel::Row row = *store->append();
        row[list_columns.net] = net_uuid;
        row[list_columns.net_class] = net.net_class->uuid;
        row[list_columns.net_name] = block.get_net_name(net_uuid);
        row[list_columns.net_class_name] = net.net_class->name;
        row[list_columns.airwires_visible] = airwire_is_visible(net_uuid);
        if (brd.airwires.count(net_uuid)) {
            row[list_columns.n_airwires] = brd.airwires.at(net_uuid).size();
        }
        else {
            row[list_columns.n_airwires] = 0;
        }
    }
    auto nc_current = netclass_combo->get_active_id();
    netclass_combo->remove_all();
    netclass_combo->append((std::string)UUID(), "All");
    for (const auto &[nc_uuid, nc] : block.net_classes) {
        netclass_combo->append((std::string)nc_uuid, nc.name);
    }
    try {
        auto uu = UUID(nc_current);
        if (block.net_classes.count(uu)) {
            netclass_combo->set_active_id(nc_current);
        }
        else {
            netclass_combo->set_active_id((std::string)UUID());
        }
    }
    catch (...) {
        netclass_combo->set_active_id((std::string)UUID());
    }
    s_signal_changed.emit();
}

bool AirwireFilterWindow::get_filtered() const
{
    return std::any_of(airwires_visible.begin(), airwires_visible.end(),
                       [](const auto &x) { return x.second == false; });
}

void AirwireFilterWindow::update_from_board()
{
    for (auto &it : store->children()) {
        Gtk::TreeModel::Row row = *it;
        if (brd.airwires.count(row[list_columns.net])) {
            row[list_columns.n_airwires] = brd.airwires.at(row[list_columns.net]).size();
        }
    }
    s_signal_changed.emit();
}

void AirwireFilterWindow::load_from_json(const json &j)
{
    for (const auto &[key, value] : j.at("airwires_visible").items()) {
        const UUID net = key;
        if (block.nets.count(net))
            airwires_visible[net] = value;
    }
    for (const auto &[key, value] : j.at("net_colors").items()) {
        const UUID net = key;
        if (block.nets.count(net))
            net_colors[net] = colori_from_json(value);
    }
    for (auto &it : store->children()) {
        Gtk::TreeModel::Row row = *it;
        const UUID net = row[list_columns.net];
        if (airwires_visible.count(net)) {
            row[list_columns.airwires_visible] = airwires_visible.at(net);
        }
        if (net_colors.count(net)) {
            row[list_columns.color] = rgba_from_colori(net_colors.at(net));
        }
    }
    s_signal_changed.emit();
}

json AirwireFilterWindow::serialize()
{
    json j;
    {
        json o;
        for (const auto &[net, v] : airwires_visible) {
            o[(std::string)net] = v;
        }
        j["airwires_visible"] = o;
    }
    {
        json o;
        for (const auto &[net, color] : net_colors) {
            o[(std::string)net] = colori_to_json(color);
        }
        j["net_colors"] = o;
    }

    return j;
}

}; // namespace horizon
