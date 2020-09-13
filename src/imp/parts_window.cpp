#include "parts_window.hpp"
#include "board/board.hpp"
#include "pool/part.hpp"
#include "util/util.hpp"
#include "util/gtk_util.hpp"
#include "nlohmann/json.hpp"

namespace horizon {
PartsWindow::PartsWindow(const Board &brd) : Gtk::Window(), board(brd), state_store(this, "parts")
{
    set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
    auto header = Gtk::manage(new Gtk::HeaderBar());
    header->set_show_close_button(true);
    header->set_title("Parts");
    set_titlebar(*header);
    header->show();
    if (!state_store.get_default_set())
        set_default_size(500, 500);

    auto sc = Gtk::manage(new Gtk::ScrolledWindow);
    sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);

    store = Gtk::ListStore::create(list_columns);
    tree_view = Gtk::manage(new Gtk::TreeView(store));
    tree_view->get_selection()->set_mode(Gtk::SELECTION_MULTIPLE);

    {
        auto cr = Gtk::manage(new Gtk::CellRendererToggle());
        cr->property_xalign() = 0;
        cr->signal_toggled().connect([this](const Glib::ustring &path) {
            auto it = store->get_iter(path);
            if (it) {
                Gtk::TreeModel::Row row = *it;
                row[list_columns.placed] = !row[list_columns.placed];
            }
        });
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Placed", *cr));
        auto col = tree_view->get_column(tree_view->append_column(*tvc) - 1);
        col->add_attribute(cr->property_active(), list_columns.placed);
    }


    tree_view->append_column("QTY", list_columns.qty);
    tree_view->get_column(1)->set_sort_column(list_columns.qty);
    tree_view->append_column("Value", list_columns.value);
    tree_view->append_column("MPN", list_columns.MPN);
    tree_view_append_column_ellipsis(tree_view, "Refdes", list_columns.refdes, Pango::ELLIPSIZE_END)
            ->set_sort_column(list_columns.refdes);
    store->set_sort_func(list_columns.refdes,
                         [this](const Gtk::TreeModel::iterator &ia, const Gtk::TreeModel::iterator &ib) {
                             Gtk::TreeModel::Row ra = *ia;
                             Gtk::TreeModel::Row rb = *ib;
                             Glib::ustring a = ra[list_columns.refdes];
                             Glib::ustring b = rb[list_columns.refdes];
                             return strcmp_natural(a, b);
                         });

    tree_view->get_selection()->signal_changed().connect([this] {
        std::set<UUID> comps_sel;
        for (const auto &path : tree_view->get_selection()->get_selected_rows()) {
            Gtk::TreeModel::Row row = *store->get_iter(path);
            const auto &comps = row.get_value(list_columns.components);
            comps_sel.insert(comps.begin(), comps.end());
        }
        s_signal_selected.emit(comps_sel);
    });

    tree_view->set_search_column(list_columns.components);
    tree_view->set_search_equal_func([this](const Glib::RefPtr<Gtk::TreeModel> &model, int c,
                                            const Glib::ustring &needle, const Gtk::TreeModel::iterator &it) {
        auto cneedle = needle.casefold();
        for (const auto &comp_uu : it->get_value(list_columns.components)) {
            const auto &comp = board.block->components.at(comp_uu);
            if (Glib::ustring(comp.refdes).casefold() == cneedle) {
                return false; // found
            }
        }
        return true; // not found
    });
    store->set_sort_column(list_columns.refdes, Gtk::SORT_ASCENDING);

    sc->add(*tree_view);
    sc->show_all();
    add(*sc);
}


void PartsWindow::update()
{
    std::set<UUID> parts_placed;
    for (const auto &it : store->children()) {
        Gtk::TreeModel::Row row = *it;
        if (row[list_columns.placed]) {
            parts_placed.insert(row[list_columns.part]);
        }
    }
    store->clear();
    std::map<const Part *, std::list<const Component *>> parts;
    for (const auto &[uu, comp] : board.block->components) {
        if (comp.part)
            parts[comp.part].push_back(&comp);
    }
    for (auto &[part, comps] : parts) {
        comps.sort([](const auto &a, const auto &b) { return strcmp_natural(a->refdes, b->refdes) < 0; });
        Gtk::TreeModel::Row row = *store->append();
        std::string refdes;
        std::set<UUID> comp_uuids;
        for (const auto &comp : comps) {
            refdes += comp->refdes;
            refdes += ", ";
            comp_uuids.insert(comp->uuid);
        }
        if (refdes.size()) {
            refdes.pop_back();
            refdes.pop_back();
        }

        row[list_columns.MPN] = part->get_MPN();
        row[list_columns.value] = part->get_value();
        row[list_columns.part] = part->uuid;
        row[list_columns.refdes] = refdes;
        row[list_columns.placed] = parts_placed.count(part->uuid);
        row[list_columns.qty] = comps.size();
        row[list_columns.components] = comp_uuids;
    }
}

json PartsWindow::serialize() const
{
    json j;
    for (const auto &it : store->children()) {
        Gtk::TreeModel::Row row = *it;
        if (row[list_columns.placed]) {
            j.push_back((std::string)row.get_value(list_columns.part));
        }
    }

    json k;
    k["parts_placed"] = j;
    return k;
}

void PartsWindow::load_from_json(const json &j)
{
    std::set<UUID> parts_placed;
    for (const auto &it : j.at("parts_placed")) {
        parts_placed.emplace(it.get<std::string>());
    }
    for (auto &it : store->children()) {
        Gtk::TreeModel::Row row = *it;
        row[list_columns.placed] = parts_placed.count(row[list_columns.part]);
    }
}


} // namespace horizon
