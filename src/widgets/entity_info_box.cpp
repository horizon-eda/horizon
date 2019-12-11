#include "entity_info_box.hpp"
#include "where_used_box.hpp"
#include "util/util.hpp"
#include "util/gtk_util.hpp"
#include "pool/entity.hpp"

namespace horizon {
EntityInfoBox::EntityInfoBox(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, Pool &p)
    : Gtk::Box(cobject), pool(p)
{

    where_used_box = Gtk::manage(new WhereUsedBox(pool));
    where_used_box->show();
    where_used_box->signal_goto().connect([this](ObjectType type, UUID uu) { s_signal_goto.emit(type, uu); });
    Gtk::Box *bo;
    x->get_widget("info_where_used_box", bo);
    bo->pack_start(*where_used_box, true, true, 0);
    where_used_box->show();

    Gtk::Grid *grid;
    x->get_widget("info_grid", grid);
    {
        auto la = Gtk::manage(new Gtk::Label("Name"));
        la->get_style_context()->add_class("dim-label");
        la->set_xalign(1);
        grid->attach(*la, 0, 0, 1, 1);
        label_name = Gtk::manage(new Gtk::Label);
        label_name->set_xalign(0);
        label_name->set_hexpand(true);
        grid->attach(*label_name, 1, 0, 1, 1);
    }
    {
        auto la = Gtk::manage(new Gtk::Label("Manufacturer"));
        la->get_style_context()->add_class("dim-label");
        la->set_xalign(1);
        grid->attach(*la, 2, 0, 1, 1);
        label_manufacturer = Gtk::manage(new Gtk::Label);
        label_manufacturer->set_xalign(0);
        label_manufacturer->set_hexpand(true);
        grid->attach(*label_manufacturer, 3, 0, 1, 1);
    }
    {
        auto la = Gtk::manage(new Gtk::Label("Prefix"));
        la->get_style_context()->add_class("dim-label");
        la->set_xalign(1);
        grid->attach(*la, 4, 0, 1, 1);
        label_prefix = Gtk::manage(new Gtk::Label);
        label_prefix->set_xalign(0);
        label_prefix->set_hexpand(true);
        grid->attach(*label_prefix, 5, 0, 1, 1);
    }
    {
        auto la = Gtk::manage(new Gtk::Label("Tags"));
        la->get_style_context()->add_class("dim-label");
        la->set_xalign(1);
        grid->attach(*la, 0, 1, 1, 1);
        label_tags = Gtk::manage(new Gtk::Label);
        label_tags->set_xalign(0);
        label_tags->set_hexpand(true);
        grid->attach(*label_tags, 1, 1, 5, 1);
    }

    x->get_widget("info_tv", view);

    store = Gtk::ListStore::create(list_columns);
    store->set_sort_func(list_columns.name,
                         [this](const Gtk::TreeModel::iterator &ia, const Gtk::TreeModel::iterator &ib) {
                             Gtk::TreeModel::Row ra = *ia;
                             Gtk::TreeModel::Row rb = *ib;
                             Glib::ustring a = ra[list_columns.name];
                             Glib::ustring b = rb[list_columns.name];
                             return strcmp_natural(a, b);
                         });


    view->set_model(store);
    {
        auto i = view->append_column("Name", list_columns.name);
        view->get_column(i - 1)->set_sort_column(list_columns.name);
    }
    view->append_column("Suffix", list_columns.suffix);
    {
        auto cr = Gtk::manage(new Gtk::CellRendererText());
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Unit", *cr));
        tvc->set_cell_data_func(*cr, [this](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
            Gtk::TreeModel::Row row = *it;
            auto mcr = dynamic_cast<Gtk::CellRendererText *>(tcr);
            const Unit *unit = row[list_columns.unit];
            mcr->property_text() = unit->name;
        });
        view->append_column(*tvc);
    }
    view->signal_row_activated().connect([this](const Gtk::TreeModel::Path &path, Gtk::TreeViewColumn *column) {
        auto it = store->get_iter(path);
        if (it) {
            Gtk::TreeModel::Row row = *it;
            const Unit *unit = row[list_columns.unit];
            s_signal_goto.emit(ObjectType::UNIT, unit->uuid);
        }
    });
}

EntityInfoBox *EntityInfoBox::create(Pool &p)
{
    EntityInfoBox *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/horizon-eda/horizon/pool-prj-mgr/window.ui", "info_box");
    x->get_widget_derived("info_box", w, p);
    w->reference();
    return w;
}

void EntityInfoBox::load(const Entity *entity)
{
    if (entity)
        where_used_box->load(ObjectType::ENTITY, entity->uuid);
    else
        where_used_box->clear();
    label_manufacturer->set_label(entity ? entity->manufacturer : "");
    label_name->set_label(entity ? entity->name : "");
    label_prefix->set_label(entity ? entity->prefix : "");
    std::string tags;
    if (entity) {
        for (const auto &it : entity->tags) {
            tags += it + " ";
        }
    }
    label_tags->set_label(tags);

    store->clear();
    if (entity) {
        for (const auto &it : entity->gates) {
            Gtk::TreeModel::Row row = *store->append();
            row[list_columns.name] = it.second.name;
            row[list_columns.suffix] = it.second.suffix;
            row[list_columns.unit] = it.second.unit.ptr;
        }
    }
}
} // namespace horizon
