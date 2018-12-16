#include "unit_info_box.hpp"
#include "where_used_box.hpp"
#include "util/util.hpp"
#include "util/gtk_util.hpp"

namespace horizon {
static const std::map<Pin::Direction, std::string> pin_direction_map = [] {
    std::map<Pin::Direction, std::string> r;
    for (const auto &it : Pin::direction_names) {
        r.emplace(it.first, it.second);
    }
    return r;
}();

UnitInfoBox::UnitInfoBox(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, Pool &p)
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


    x->get_widget("info_tv", view);

    store = Gtk::ListStore::create(list_columns);
    store->set_sort_func(list_columns.primary_name,
                         [this](const Gtk::TreeModel::iterator &ia, const Gtk::TreeModel::iterator &ib) {
                             Gtk::TreeModel::Row ra = *ia;
                             Gtk::TreeModel::Row rb = *ib;
                             Glib::ustring a = ra[list_columns.primary_name];
                             Glib::ustring b = rb[list_columns.primary_name];
                             return strcmp_natural(a, b);
                         });


    view->set_model(store);
    {
        auto cr = Gtk::manage(new Gtk::CellRendererText());
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Direction", *cr));
        tvc->set_cell_data_func(*cr, [this](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
            Gtk::TreeModel::Row row = *it;
            auto mcr = dynamic_cast<Gtk::CellRendererText *>(tcr);
            mcr->property_text() = pin_direction_map.at(row[list_columns.direction]);
        });
        view->append_column(*tvc);
    }
    {
        auto i = view->append_column("Name", list_columns.primary_name);
        view->get_column(i - 1)->set_sort_column(list_columns.primary_name);
    }
    // view->append_column("Swap group", list_columns.swap_group);
    view->append_column("Alt. names", list_columns.alt_names);
}

UnitInfoBox *UnitInfoBox::create(Pool &p)
{
    UnitInfoBox *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/net/carrotIndustries/horizon/pool-prj-mgr/window.ui", "info_box");
    x->get_widget_derived("info_box", w, p);
    w->reference();
    return w;
}

void UnitInfoBox::load(const Unit *unit)
{
    if (unit)
        where_used_box->load(ObjectType::UNIT, unit->uuid);
    else
        where_used_box->clear();
    label_manufacturer->set_label(unit ? unit->manufacturer : "");
    label_name->set_label(unit ? unit->name : "");

    store->clear();
    if (unit) {
        for (const auto &it : unit->pins) {
            Gtk::TreeModel::Row row = *store->append();
            row[list_columns.direction] = it.second.direction;
            row[list_columns.primary_name] = it.second.primary_name;
            std::string alts;
            for (const auto &it_alt : it.second.names) {
                alts += it_alt + ", ";
            }
            if (alts.size()) {
                alts.pop_back();
                alts.pop_back();
            }
            row[list_columns.alt_names] = alts;
        }
    }
}
} // namespace horizon
