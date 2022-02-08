#include "multi_net_selector.hpp"
#include "block/block.hpp"
#include "util/util.hpp"

namespace horizon {
MultiNetSelector::MultiNetSelector(const Block &b) : block(b)
{
    store = Gtk::ListStore::create(list_columns);
    filter_available = Gtk::TreeModelFilter::create(store);
    filter_available->set_visible_func([this](const Gtk::TreeModel::const_iterator &it) -> bool {
        const Gtk::TreeModel::Row row = *it;
        return nets_selected.count(row.get_value(list_columns.uuid)) == 0;
    });
    sort_available = Gtk::TreeModelSort::create(filter_available);

    filter_selected = Gtk::TreeModelFilter::create(store);
    filter_selected->set_visible_func([this](const Gtk::TreeModel::const_iterator &it) -> bool {
        const Gtk::TreeModel::Row row = *it;
        return nets_selected.count(row.get_value(list_columns.uuid)) != 0;
    });
    sort_selected = Gtk::TreeModelSort::create(filter_selected);
    set_row_spacing(4);
    set_column_spacing(4);

    sg_views = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);

    {
        auto la = Gtk::manage(new Gtk::Label);
        la->set_markup("<b>Available</b>");
        la->get_style_context()->add_class("dim-label");
        attach(*la, 0, 0, 1, 1);
        la->show();
    }
    {
        auto la = Gtk::manage(new Gtk::Label);
        la->set_markup("<b>Selected</b>");
        la->get_style_context()->add_class("dim-label");
        attach(*la, 2, 0, 1, 1);
        la->show();
    }

    {
        auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 8));
        box->set_vexpand(true);
        box->set_valign(Gtk::ALIGN_CENTER);

        button_include = Gtk::manage(new Gtk::Button);
        button_include->set_sensitive(false);
        button_include->signal_clicked().connect([this] {
            for (auto &path : view_available->get_selection()->get_selected_rows()) {
                Gtk::TreeModel::Row row = *view_available->get_model()->get_iter(path);
                nets_selected.insert(row.get_value(list_columns.uuid));
            }
            filter_available->refilter();
            filter_selected->refilter();
            s_signal_changed.emit();
        });
        button_include->set_image_from_icon_name("go-next-symbolic", Gtk::ICON_SIZE_BUTTON);
        box->pack_start(*button_include, false, false, 0);

        button_exclude = Gtk::manage(new Gtk::Button);
        button_exclude->set_sensitive(false);
        button_exclude->signal_clicked().connect([this] {
            for (auto &path : view_selected->get_selection()->get_selected_rows()) {
                Gtk::TreeModel::Row row = *view_selected->get_model()->get_iter(path);
                nets_selected.erase(row.get_value(list_columns.uuid));
            }
            filter_available->refilter();
            filter_selected->refilter();
            s_signal_changed.emit();
        });
        button_exclude->set_image_from_icon_name("go-previous-symbolic", Gtk::ICON_SIZE_BUTTON);
        box->pack_start(*button_exclude, false, false, 0);

        attach(*box, 1, 1, 1, 1);
        box->show_all();
    }

    {
        auto sc = make_listview(view_available, sort_available);
        sc->show();
        attach(*sc, 0, 1, 1, 1);
    }
    {
        auto sc = make_listview(view_selected, sort_selected);
        sc->show();
        attach(*sc, 2, 1, 1, 1);
    }

    view_available->get_selection()->signal_changed().connect(
            [this] { button_include->set_sensitive(view_available->get_selection()->count_selected_rows()); });
    view_selected->get_selection()->signal_changed().connect(
            [this] { button_exclude->set_sensitive(view_selected->get_selection()->count_selected_rows()); });

    view_available->signal_row_activated().connect([this](const Gtk::TreeModel::Path &path, Gtk::TreeViewColumn *tvc) {
        Gtk::TreeModel::Row row = *view_available->get_model()->get_iter(path);
        nets_selected.insert(row.get_value(list_columns.uuid));
        filter_available->refilter();
        filter_selected->refilter();
        s_signal_changed.emit();
    });

    view_selected->signal_row_activated().connect([this](const Gtk::TreeModel::Path &path, Gtk::TreeViewColumn *tvc) {
        Gtk::TreeModel::Row row = *view_selected->get_model()->get_iter(path);
        nets_selected.erase(row.get_value(list_columns.uuid));
        filter_available->refilter();
        filter_selected->refilter();
        s_signal_changed.emit();
    });

    update();
}

void MultiNetSelector::update()
{
    store->clear();
    for (const auto &[uu, net] : block.nets) {
        if (net.is_named()) {
            Gtk::TreeModel::Row row = *(store->append());
            row[list_columns.name] = net.name;
            row[list_columns.uuid] = net.uuid;
        }
    }
}

void MultiNetSelector::select_nets(const std::set<UUID> &nets)
{
    nets_selected = nets;
    filter_available->refilter();
    filter_selected->refilter();
}

std::set<UUID> MultiNetSelector::get_selected_nets() const
{
    return nets_selected;
}

Gtk::Widget *MultiNetSelector::make_listview(Gtk::TreeView *&view, Glib::RefPtr<Gtk::TreeModelSort> &model)
{

    view = Gtk::manage(new Gtk::TreeView(model));
    view->get_selection()->set_mode(Gtk::SELECTION_MULTIPLE);
    view->set_rubber_banding(true);
    view->append_column("Net", list_columns.name);
    view->get_column(0)->set_sort_column(list_columns.name);
    store->set_sort_column(list_columns.name, Gtk::SORT_ASCENDING);
    store->set_sort_func(list_columns.name,
                         [this](const Gtk::TreeModel::iterator &ia, const Gtk::TreeModel::iterator &ib) {
                             Gtk::TreeModel::Row ra = *ia;
                             Gtk::TreeModel::Row rb = *ib;
                             Glib::ustring a = ra[list_columns.name];
                             Glib::ustring b = rb[list_columns.name];
                             return strcmp_natural(a, b);
                         });

    view->show();
    auto sc = Gtk::manage(new Gtk::ScrolledWindow());
    sc->set_shadow_type(Gtk::SHADOW_IN);
    sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    sc->add(*view);
    sg_views->add_widget(*sc);

    return sc;
}

} // namespace horizon
