#include "title_block_values_editor.hpp"
#include "dialogs/dialogs.hpp"
#include <algorithm>
#include <iostream>

namespace horizon {
TitleBlockValuesEditor::TitleBlockValuesEditor(std::map<std::string, std::string> &v)
    : Gtk::Box(Gtk::Orientation::ORIENTATION_VERTICAL, 0), values(v)
{
    store = Gtk::ListStore::create(list_columns);
    view = Gtk::manage(new Gtk::TreeView(store));

    {
        auto cr = Gtk::manage(new Gtk::CellRendererText());
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Key", *cr));
        tvc->add_attribute(*cr, "text", list_columns.key);
        view->append_column(*tvc);
    }
    {
        auto cr = Gtk::manage(new Gtk::CellRendererText());
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Value", *cr));
        tvc->add_attribute(*cr, "text", list_columns.value);
        cr->property_editable().set_value(true);
        cr->signal_edited().connect([this](const Glib::ustring &path, const Glib::ustring &new_text) {
            auto it = store->get_iter(path);
            if (it) {
                Gtk::TreeModel::Row row = *it;
                row[list_columns.value] = new_text;
                Glib::ustring key = row[list_columns.key];
                values[key] = new_text;
            }
        });
        view->append_column(*tvc);
    }

    for (const auto &it : values) {
        auto row = *store->append();
        row[list_columns.key] = it.first;
        row[list_columns.value] = it.second;
    }


    view->show();
    auto sc = Gtk::manage(new Gtk::ScrolledWindow());
    sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    sc->set_shadow_type(Gtk::SHADOW_NONE);
    sc->set_min_content_height(150);
    sc->set_propagate_natural_height(true);
    sc->add(*view);
    sc->show_all();


    {
        auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
        sep->show();
        pack_start(*sep, false, false, 0);
    }

    pack_start(*sc, true, true, 0);

    {
        auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
        sep->show();
        pack_start(*sep, false, false, 0);
    }

    auto tb = Gtk::manage(new Gtk::Toolbar());
    tb->set_icon_size(Gtk::ICON_SIZE_MENU);
    tb->set_toolbar_style(Gtk::TOOLBAR_ICONS);
    {
        auto tbo = Gtk::manage(new Gtk::ToolButton());
        tbo->set_icon_name("list-add-symbolic");
        tbo->signal_clicked().connect([this] {
            Dialogs dias;
            auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));
            dias.set_parent(top);
            if (auto r = dias.ask_datum_string("Enter Key", "")) {
                auto row = *store->append();
                if (values.count(*r) == 0) {
                    row[list_columns.key] = *r;
                    values[*r] = "";
                }
            }
        });
        tb->insert(*tbo, -1);
    }
    {
        auto tbo = Gtk::manage(new Gtk::ToolButton());
        tbo->set_icon_name("list-remove-symbolic");
        tbo->signal_clicked().connect([this] {
            auto it = view->get_selection()->get_selected();
            if (it) {
                Gtk::TreeModel::Row row = *it;
                Glib::ustring key = row[list_columns.key];
                values.erase(key);
                store->erase(it);
            }
        });

        tb->insert(*tbo, -1);
        tb_remove = tbo;
    }

    tb_remove->set_sensitive(view->get_selection()->count_selected_rows());
    view->get_selection()->signal_changed().connect(
            [this] { tb_remove->set_sensitive(view->get_selection()->count_selected_rows()); });

    tb->show_all();
    pack_start(*tb, false, false, 0);
}

} // namespace horizon
