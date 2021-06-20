#include "grids_window.hpp"
#include "grid_controller.hpp"
#include "util/util.hpp"
#include "util/gtk_util.hpp"
#include "util/str_util.hpp"
#include "dialogs/dialogs.hpp"
#include "util/geom_util.hpp"

namespace horizon {
GridsWindow *GridsWindow::create(Gtk::Window *p, GridController &c, GridSettings &settings)
{
    GridsWindow *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/horizon-eda/horizon/imp/grids.ui");
    x->get_widget_derived("window", w, c, settings);
    w->set_transient_for(*p);
    return w;
}

void GridsWindow::row_from_grid(Gtk::TreeModel::Row &row, const GridSettings::Grid &g) const
{
    row[list_columns.uuid] = g.uuid;
    row[list_columns.name] = g.name;
    {
        std::string label;
        if (g.mode == GridSettings::Grid::Mode::SQUARE) {
            label = dim_to_string(g.spacing_square, false);
        }
        else {
            label = coord_to_string(g.spacing_rect, true);
        }
        row[list_columns.spacing] = label;
    }
    {
        std::string label;
        if (g.origin.x || g.origin.y) {
            label = coord_to_string(g.origin);
        }
        else {
            label = "Zero";
        }
        row[list_columns.origin] = label;
    }
}

GridsWindow::GridsWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, GridController &c,
                         GridSettings &setg)
    : Gtk::Window(cobject), grid_controller(c), settings(setg)
{
    install_esc_to_close(*this);

    GET_WIDGET(treeview);
    GET_WIDGET(button_box);
    GET_WIDGET(button_ok);
    GET_WIDGET(button_cancel);
    GET_WIDGET(headerbar);

    store = Gtk::ListStore::create(list_columns);

    treeview->set_model(store);
    {
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Grid"));
        auto cr = Gtk::manage(new Gtk::CellRendererText());
        tvc->pack_start(*cr, true);
        cr->property_editable() = true;
        tvc->add_attribute(cr->property_text(), list_columns.name);
        tvc->set_sort_column(list_columns.name);
        treeview->append_column(*tvc);
        cr->signal_edited().connect([this](const Glib::ustring &path, const Glib::ustring &new_text) {
            auto it = store->get_iter(path);
            if (it) {
                Gtk::TreeModel::Row row = *it;
                row[list_columns.name] = new_text;
                settings.grids.at(row[list_columns.uuid]).name = new_text;
            }
        });
    }
    {
        auto cr = Gtk::manage(new Gtk::CellRendererText());
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Spacing", *cr));
        {
            auto attributes_list = pango_attr_list_new();
            auto attribute_font_features = pango_attr_font_features_new("tnum 1");
            pango_attr_list_insert(attributes_list, attribute_font_features);
            g_object_set(G_OBJECT(cr->gobj()), "attributes", attributes_list, NULL);
        }
        tvc->add_attribute(cr->property_text(), list_columns.spacing);

        /* tvc->set_cell_data_func(*cr, [this](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
             Gtk::TreeModel::Row row = *it;
             auto mcr = dynamic_cast<Gtk::CellRendererText *>(tcr);
             const auto &s = row.get_value(list_columns.settings);

             std::string label;
             if (s.mode == GridSettings::Mode::SQUARE) {
                 label = dim_to_string(s.spacing_square, false);
             }
             else {
                 label = coord_to_string(s.spacing_rect, true);
             }

             mcr->property_text() = label;
         });*/
        treeview->append_column(*tvc);
    }
    {
        auto cr = Gtk::manage(new Gtk::CellRendererText());
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Origin", *cr));
        {
            auto attributes_list = pango_attr_list_new();
            auto attribute_font_features = pango_attr_font_features_new("tnum 1");
            pango_attr_list_insert(attributes_list, attribute_font_features);
            g_object_set(G_OBJECT(cr->gobj()), "attributes", attributes_list, NULL);
        }
        /*   tvc->set_cell_data_func(*cr, [this](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
               Gtk::TreeModel::Row row = *it;
               auto mcr = dynamic_cast<Gtk::CellRendererText *>(tcr);
               const auto &s = row.get_value(list_columns.settings);

               std::string label;
               if (s.origin.x || s.origin.y) {
                   label = coord_to_string(s.origin);
               }
               else {
                   label = "Zero";
               }

               mcr->property_text() = label;
           });*/
        tvc->add_attribute(cr->property_text(), list_columns.origin);
        treeview->append_column(*tvc);
    }

    Gtk::Button *button_create;
    GET_WIDGET(button_create);
    button_create->signal_clicked().connect([this] {
        Dialogs dias;
        dias.set_parent(this);
        if (auto r = dias.ask_datum_string("Grid name", "")) {
            const auto uu = UUID::random();
            auto &gr = settings.grids.emplace(uu, uu).first->second;
            gr.name = *r;
            gr.assign(settings.current);
            Gtk::TreeModel::Row row = *store->append();
            row_from_grid(row, gr);
            s_signal_changed.emit();
        }
    });
    Gtk::Button *button_remove;
    GET_WIDGET(button_remove);
    button_remove->signal_clicked().connect([this] {
        auto paths = treeview->get_selection()->get_selected_rows();
        for (auto it = paths.rbegin(); it != paths.rend(); it++) {
            auto iter = store->get_iter(*it);
            Gtk::TreeModel::Row row = *iter;
            settings.grids.erase(row[list_columns.uuid]);
            store->erase(iter);
        }
        s_signal_changed.emit();
    });

    Gtk::Button *button_apply;
    GET_WIDGET(button_apply);
    button_apply->signal_clicked().connect([this] {
        if (auto it = treeview->get_selection()->get_selected()) {
            Gtk::TreeModel::Row row = *it;
            grid_controller.apply_settings(settings.grids.at(row[list_columns.uuid]));
        }
    });
    button_ok->signal_clicked().connect([this] {
        if (auto it = treeview->get_selection()->get_selected()) {
            Gtk::TreeModel::Row row = *it;
            grid_controller.apply_settings(settings.grids.at(row[list_columns.uuid]));
            close();
        }
    });
    button_cancel->signal_clicked().connect([this] { close(); });

    Gtk::Button *button_load;
    GET_WIDGET(button_load);
    button_load->signal_clicked().connect([this] {
        if (auto it = treeview->get_selection()->get_selected()) {
            Gtk::TreeModel::Row row = *it;
            auto &gr = settings.grids.at(row[list_columns.uuid]);
            gr.assign(settings.current);
            row_from_grid(row, gr);
        }
    });

    treeview->signal_row_activated().connect([this](const Gtk::TreeModel::Path &path, Gtk::TreeViewColumn *col) {
        Gtk::TreeModel::Row row = *store->get_iter(path);
        grid_controller.apply_settings(settings.grids.at(row[list_columns.uuid]));
        if (select_mode)
            close();
    });
    treeview->signal_key_press_event().connect([this](GdkEventKey *ev) {
        if (ev->keyval == GDK_KEY_Delete) {
            auto paths = treeview->get_selection()->get_selected_rows();
            for (auto it = paths.rbegin(); it != paths.rend(); it++)
                store->erase(store->get_iter(*it));
            s_signal_changed.emit();
            return true;
        }
        return false;
    });

    for (const auto &[uu, gr] : settings.grids) {
        Gtk::TreeModel::Row row = *store->append();
        row_from_grid(row, gr);
    }

    set_select_mode(false);
}

void GridsWindow::set_select_mode(bool m)
{
    select_mode = m;
    headerbar->set_show_close_button(!select_mode);
    button_ok->set_visible(select_mode);
    button_cancel->set_visible(select_mode);
    button_box->set_visible(!select_mode);
    set_modal(select_mode);

    if (select_mode) {
        set_title("Select Grid");
        if (auto it = treeview->get_selection()->get_selected(); !it) {
            if (store->children().size()) {
                treeview->get_selection()->select(store->children().begin());
            }
        }
        if (auto it = treeview->get_selection()->get_selected()) {
            auto path = store->get_path(it);
            treeview->set_cursor(path, *treeview->get_column(1));
        }
    }
    else {
        set_title("Grids");
    }
    treeview->grab_focus();
}

bool GridsWindow::has_grids() const
{
    return settings.grids.size();
}

/*
void GridsWindow::load_from_json(const json &j)
{
    store->clear();
    for (const auto &it : j) {
        Gtk::TreeModel::Row row = *store->append();
        row[list_columns.name] = it.at("name").get<std::string>();
        row[list_columns.settings] = GridSettings(it);
    }
    s_signal_changed.emit();
}

json GridsWindow::serialize()
{
    json j = json::array();
    for (const auto &it : store->children()) {
        Gtk::TreeModel::Row row = *it;
        json o = row.get_value(list_columns.settings).serialize();
        o["name"] = std::string(row.get_value(list_columns.name));
        j.push_back(o);
    }
    return j;
}
*/

}; // namespace horizon
