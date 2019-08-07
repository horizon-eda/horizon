#include "where_used_box.hpp"
#include "pool/pool.hpp"
#include "common/object_descr.hpp"

namespace horizon {
WhereUsedBox::WhereUsedBox(Pool &p) : Gtk::Box(Gtk::Orientation::ORIENTATION_VERTICAL, 4), pool(p)
{
    auto *la = Gtk::manage(new Gtk::Label());
    la->set_markup("<b>Where used</b>");
    la->set_xalign(0);
    la->show();
    pack_start(*la, false, false, 0);

    store = Gtk::ListStore::create(list_columns);
    view = Gtk::manage(new Gtk::TreeView(store));
    {
        auto cr = Gtk::manage(new Gtk::CellRendererText());
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Type", *cr));
        tvc->set_cell_data_func(*cr, [this](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
            Gtk::TreeModel::Row row = *it;
            auto mcr = dynamic_cast<Gtk::CellRendererText *>(tcr);
            mcr->property_text() = object_descriptions.at(row[list_columns.type]).name;
        });
        view->append_column(*tvc);
    }
    view->append_column("Name", list_columns.name);
    view->show();
    auto sc = Gtk::manage(new Gtk::ScrolledWindow());
    sc->set_shadow_type(Gtk::SHADOW_IN);
    sc->set_min_content_height(100);
    sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    sc->add(*view);
    sc->show_all();

    view->signal_row_activated().connect(sigc::mem_fun(*this, &WhereUsedBox::row_activated));
    pack_start(*sc, true, true, 0);
    set_visible(false);
}

void WhereUsedBox::clear()
{
    store->clear();
}

void WhereUsedBox::load(ObjectType type, const UUID &uu)
{
    store->clear();
    if (!uu)
        return;
    SQLite::Query q(pool.db,
                    "WITH RECURSIVE where_used(typex, uuidx) AS ( SELECT ?, ? UNION "
                    "SELECT type, uuid FROM dependencies, where_used "
                    "WHERE dependencies.dep_type = where_used.typex "
                    "AND dependencies.dep_uuid = where_used.uuidx) "
                    "SELECT where_used.typex, where_used.uuidx, all_items_view.name "
                    "FROM where_used "
                    "LEFT JOIN all_items_view "
                    "ON where_used.typex = all_items_view.type "
                    "AND where_used.uuidx = all_items_view.uuid;");
    q.bind(1, object_type_lut.lookup_reverse(type));
    q.bind(2, uu);
    q.step(); // drop first one
    while (q.step()) {
        std::string type_str = q.get<std::string>(0);
        UUID uuid = q.get<std::string>(1);
        std::string name = q.get<std::string>(2);

        Gtk::TreeModel::Row row;
        row = *store->append();
        row[list_columns.type] = object_type_lut.lookup(type_str);
        row[list_columns.uuid] = uuid;
        row[list_columns.name] = name;
    }
}

void WhereUsedBox::row_activated(const Gtk::TreeModel::Path &path, Gtk::TreeViewColumn *column)
{
    auto it = store->get_iter(path);
    if (it) {
        Gtk::TreeModel::Row row = *it;
        s_signal_goto.emit(row[list_columns.type], row[list_columns.uuid]);
    }
}

} // namespace horizon
