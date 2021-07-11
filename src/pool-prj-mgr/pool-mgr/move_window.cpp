#include "move_window.hpp"
#include "pool/pool.hpp"
#include "util/gtk_util.hpp"
#include "common/object_descr.hpp"
#include "widgets/location_entry.hpp"
#include "pool/pool_manager.hpp"

namespace horizon {
MoveWindow *MoveWindow::create(Pool &pool, ObjectType type, const UUID &uu)
{
    MoveWindow *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/horizon-eda/horizon/pool-prj-mgr/pool-mgr/move_window.ui");
    x->get_widget_derived("window", w, pool, type, uu);

    return w;
}

class MoveItemRow : public Gtk::Box {
public:
    MoveItemRow(MoveWindow &p, SQLite::Query &q)
        : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 10), parent(p), filename(q.get<std::string>(0))
    {
        property_margin() = 5;
        set_margin_end(10);
        const auto type = q.get<ObjectType>(1);
        const auto name = q.get<std::string>(2);
        cb_item = Gtk::manage(new Gtk::CheckButton());
        {
            auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 5));
            {
                auto la = Gtk::manage(new Gtk::Label(object_descriptions.at(type).name));
                la->set_xalign(0);
                parent.sg_type->add_widget(*la);
                box->pack_start(*la, false, false, 0);
            }
            {
                auto la = Gtk::manage(new Gtk::Label(name));
                la->set_xalign(0);
                box->pack_start(*la, false, false, 0);
            }
            cb_item->add(*box);
        }
        cb_item->show_all();
        cb_item->set_active(true);
        parent.sg_item->add_widget(*cb_item);
        pack_start(*cb_item, false, false, 0);


        {
            auto la = Gtk::manage(new Gtk::Label(filename));
            la->set_xalign(0);
            la->show();
            parent.sg_src->add_widget(*la);
            pack_start(*la, false, false, 0);
        }

        entry = Gtk::manage(new LocationEntry);
        if (type != ObjectType::MODEL_3D)
            entry->set_append_json(true);
        entry->show();
        entry->set_rel_filename(filename);
        parent.sg_dest->add_widget(*entry);
        pack_start(*entry, true, true, 0);
    }

    MoveWindow &parent;
    Gtk::CheckButton *cb_item;
    LocationEntry *entry;
    const std::string filename;
};


MoveWindow::MoveWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, Pool &apool, ObjectType type,
                       const UUID &uu)
    : Gtk::Window(cobject), pool(apool), window_state_store(this, "move-pool-item")
{
    GET_OBJECT(sg_item);
    GET_OBJECT(sg_src);
    GET_OBJECT(sg_dest);
    sg_type = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);

    SQLite::Query q(pool.get_db(),
                    "WITH RECURSIVE "
                    "deps(typex, uuidx) AS "
                    "( SELECT $type, $uuid UNION "
                    "SELECT dep_type, dep_uuid FROM dependencies, deps "
                    "WHERE dependencies.type = deps.typex AND dependencies.uuid = deps.uuidx) "
                    ", where_used(typex, uuidx) AS ( SELECT $type, $uuid UNION "
                    "SELECT type, uuid FROM dependencies, where_used "
                    "WHERE dependencies.dep_type = where_used.typex "
                    "AND dependencies.dep_uuid = where_used.uuidx) "

                    "SELECT filename, x.type, x.uuid FROM "
                    "(SELECT typex as type, uuidx as uuid FROM deps "
                    "UNION SELECT 'symbol' as type, symbols.uuid as uuid FROM symbols "
                    "INNER JOIN deps ON (symbols.unit = uuidx AND typex = 'unit')) AS x "
                    "LEFT JOIN all_items_view "
                    "ON (all_items_view.type = x.type AND all_items_view.uuid = x.uuid) "
                    "WHERE all_items_view.pool_uuid = $pool "

                    "UNION SELECT model_filename, 'model_3d', '' FROM models INNER JOIN deps "
                    "ON (deps.typex = 'package' AND deps.uuidx = models.package_uuid) "
                    "LEFT JOIN packages ON (models.package_uuid = packages.uuid) "
                    "WHERE packages.pool_uuid = $pool "

                    "UNION SELECT filename, type, name FROM where_used "
                    "LEFT JOIN all_items_view "
                    "ON (where_used.typex = all_items_view.type "
                    "AND where_used.uuidx = all_items_view.uuid) "
                    "WHERE all_items_view.pool_uuid = $pool");
    q.bind("$type", type);
    q.bind("$uuid", uu);
    q.bind("$pool", pool.get_pool_info().uuid);
    GET_WIDGET(listbox);

    while (q.step()) {
        auto w = Gtk::manage(new MoveItemRow(*this, q));
        w->show();
        listbox->append(*w);
    }

    GET_WIDGET(pool_combo);
    for (const auto &[path, it] : PoolManager::get().get_pools()) {
        if (path != pool.get_base_path()) {
            pool_combo->append(path, it.name + " (" + path + ")");
        }
    }
    pool_combo->signal_changed().connect([this] {
        const std::string path = pool_combo->get_active_id();
        for (auto ch : listbox->get_children()) {
            if (auto row = dynamic_cast<Gtk::ListBoxRow *>(ch)) {
                if (auto w = dynamic_cast<MoveItemRow *>(row->get_child())) {
                    w->entry->set_relative_to(path);
                }
            }
        }
    });
    pool_combo->set_active(0);

    {
        Gtk::Button *button_move;
        GET_WIDGET(button_move);
        button_move->signal_clicked().connect(sigc::mem_fun(*this, &MoveWindow::do_move));
    }
}

void MoveWindow::do_move()
{
    for (auto ch : listbox->get_children()) {
        if (auto row = dynamic_cast<Gtk::ListBoxRow *>(ch)) {
            if (auto w = dynamic_cast<MoveItemRow *>(row->get_child())) {
                if (w->cb_item->get_active()) {
                    auto src = Gio::File::create_for_path(Glib::build_filename(pool.get_base_path(), w->filename));
                    auto dest = Gio::File::create_for_path(w->entry->get_filename());
                    try {
                        dest->get_parent()->make_directory_with_parents();
                    }
                    catch (Gio::Error &e) {
                        if (e.code() != Gio::Error::EXISTS)
                            throw;
                    }
                    src->move(dest);
                }
            }
        }
    }
    moved = true;
    hide();
}

} // namespace horizon
