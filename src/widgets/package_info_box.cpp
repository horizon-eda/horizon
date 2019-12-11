#include "package_info_box.hpp"
#include "where_used_box.hpp"
#include "util/util.hpp"
#include "util/gtk_util.hpp"
#include "pool/package.hpp"
#include "pool/pool.hpp"

namespace horizon {
PackageInfoBox::PackageInfoBox(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, Pool &p)
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
        auto la = Gtk::manage(new Gtk::Label("Alt. for"));
        la->get_style_context()->add_class("dim-label");
        la->set_xalign(1);
        grid->attach(*la, 4, 0, 1, 1);
        label_alt_for = Gtk::manage(new Gtk::Label);
        label_alt_for->set_xalign(0);
        label_alt_for->set_hexpand(true);
        grid->attach(*label_alt_for, 5, 0, 1, 1);
        label_alt_for->signal_activate_link().connect(
                [this](const std::string &url) {
                    s_signal_goto.emit(ObjectType::PACKAGE, UUID(url));
                    return true;
                },
                false);
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


    view->set_model(store);
    {
        auto cr = Gtk::manage(new Gtk::CellRendererText());
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Padstack", *cr));
        tvc->set_cell_data_func(*cr, [this](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
            Gtk::TreeModel::Row row = *it;
            auto mcr = dynamic_cast<Gtk::CellRendererText *>(tcr);
            const Padstack *padstack = row[list_columns.padstack];
            mcr->property_text() = padstack->name;
        });
        view->append_column(*tvc);
    }
    view->append_column("Count", list_columns.count);
    {
        auto cr = Gtk::manage(new Gtk::CellRendererText());
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Type", *cr));
        tvc->set_cell_data_func(*cr, [this](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
            Gtk::TreeModel::Row row = *it;
            auto mcr = dynamic_cast<Gtk::CellRendererText *>(tcr);
            if (row[list_columns.specific]) {
                mcr->property_text() = "Specific";
            }
            else {
                mcr->property_text() = "Global";
            }
        });
        view->append_column(*tvc);
    }

    view->signal_row_activated().connect([this](const Gtk::TreeModel::Path &path, Gtk::TreeViewColumn *column) {
        auto it = store->get_iter(path);
        if (it) {
            Gtk::TreeModel::Row row = *it;
            const Padstack *padstack = row[list_columns.padstack];
            s_signal_goto.emit(ObjectType::PADSTACK, padstack->uuid);
        }
    });
}

PackageInfoBox *PackageInfoBox::create(Pool &p)
{
    PackageInfoBox *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/horizon-eda/horizon/pool-prj-mgr/window.ui", "info_box");
    x->get_widget_derived("info_box", w, p);
    w->reference();
    return w;
}

void PackageInfoBox::load(const Package *package)
{
    if (package)
        where_used_box->load(ObjectType::PACKAGE, package->uuid);
    else
        where_used_box->clear();
    label_manufacturer->set_label(package ? package->manufacturer : "");
    label_name->set_label(package ? package->name : "");
    if (package) {
        if (package->alternate_for)
            label_alt_for->set_markup("<a href=\"" + (std::string)package->alternate_for->uuid + "\">"
                                      + Glib::Markup::escape_text(package->alternate_for->name) + "</a>");
        else
            label_alt_for->set_markup("<i>None</i>");
    }
    else {
        label_alt_for->set_text("");
    }
    std::string tags;
    if (package) {
        for (const auto &it : package->tags) {
            tags += it + " ";
        }
    }
    label_tags->set_label(tags);

    store->clear();
    if (package) {
        std::map<const Padstack *, std::pair<unsigned int, bool>> padstacks;
        {
            SQLite::Query q(pool.db, "SELECT uuid FROM padstacks WHERE package=?");
            q.bind(1, package->uuid);
            while (q.step()) {
                UUID uu = q.get<std::string>(0);
                padstacks[pool.get_padstack(uu)] = {0, true};
            }
        }
        for (const auto &it : package->pads) {
            padstacks[it.second.pool_padstack].first++;
        }
        for (const auto &it : padstacks) {
            Gtk::TreeModel::Row row = *store->append();
            row[list_columns.padstack] = it.first;
            row[list_columns.count] = it.second.first;
            row[list_columns.specific] = it.second.second;
        }
    }
}
} // namespace horizon
