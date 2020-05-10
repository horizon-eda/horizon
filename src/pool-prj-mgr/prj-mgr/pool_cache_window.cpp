#include <widgets/cell_renderer_color_box.hpp>
#include "pool_cache_window.hpp"
#include "common/object_descr.hpp"
#include "pool/part.hpp"
#include "pool/pool_cached.hpp"
#include "util/gtk_util.hpp"
#include "util/util.hpp"
#include <fstream>
#include "nlohmann/json.hpp"
#include "pool-prj-mgr/pool-prj-mgr-app_win.hpp"
#include <giomm/file.h>
#include "pool/pool_manager.hpp"
#include <iomanip>

namespace horizon {
PoolCacheWindow *PoolCacheWindow::create(Gtk::Window *p, const std::string &cache_path, const std::string &pool_path,
                                         PoolProjectManagerAppWindow *aw)
{
    PoolCacheWindow *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/horizon-eda/horizon/pool-prj-mgr/prj-mgr/pool_cache_window.ui");
    x->get_widget_derived("window", w, cache_path, pool_path, aw);

    w->set_transient_for(*p);

    return w;
}

void PoolCacheWindow::refresh_list(const PoolCacheStatus &status)
{
    pool.clear();
    std::set<UUID> selected_uuids;
    {
        auto rows = pool_item_view->get_selection()->get_selected_rows();
        for (const auto &path : rows) {
            Gtk::TreeModel::Row row = *item_store->get_iter(path);
            selected_uuids.insert(row[tree_columns.uuid]);
        }
    }

    item_store->freeze_notify();
    item_store->clear();
    for (const auto &it : status.items) {
        Gtk::TreeModel::Row row = *item_store->append();
        row[tree_columns.name] = it.name;
        row[tree_columns.type] = it.type;
        row[tree_columns.uuid] = it.uuid;
        row[tree_columns.state] = it.state;
        row[tree_columns.delta] = it.delta;
        row[tree_columns.filename_cached] = it.filename_cached;
    }
    item_store->thaw_notify();
    for (const auto &it : item_store->children()) {
        Gtk::TreeModel::Row row = *it;
        if (selected_uuids.count(row[tree_columns.uuid])) {
            pool_item_view->get_selection()->select(it);
        }
    }
    tree_view_scroll_to_selection(pool_item_view);
    std::stringstream ss;
    if (status.n_total >= 0) {
        ss << "Total:" << status.n_total << " ";
        ss << "Current:" << status.n_current << " ";
        if (status.n_out_of_date)
            ss << "Out of date:" << status.n_out_of_date << " ";
        if (status.n_missing)
            ss << "Missing:" << status.n_missing;
    }
    else {
        ss << "Database was busy";
    }
    status_label->set_text(ss.str());
}

void PoolCacheWindow::update_from_pool()
{
    auto rows = pool_item_view->get_selection()->get_selected_rows();
    for (const auto &path : rows) {
        Gtk::TreeModel::Row row = *item_store->get_iter(path);
        PoolCacheStatus::Item::State state = row[tree_columns.state];
        ObjectType type = row[tree_columns.type];
        if (state == PoolCacheStatus::Item::State::OUT_OF_DATE && type == ObjectType::MODEL_3D) {
            std::string model_path = Glib::build_filename("3d_models", row[tree_columns.filename_cached]);
            auto dst = Gio::File::create_for_path(Glib::build_filename(cache_path, model_path));
            auto src = Gio::File::create_for_path(Glib::build_filename(pool.get_base_path(), model_path));
            src->copy(dst, Gio::FILE_COPY_OVERWRITE);
        }
        else if (state == PoolCacheStatus::Item::State::OUT_OF_DATE) {
            auto dst = Gio::File::create_for_path(row[tree_columns.filename_cached]);
            auto src = Gio::File::create_for_path(pool.get_filename(row[tree_columns.type], row[tree_columns.uuid]));
            src->copy(dst, Gio::FILE_COPY_OVERWRITE);
        }
    }
    appwin->update_pool_cache_status_now();
}

void PoolCacheWindow::selection_changed()
{
    auto rows = pool_item_view->get_selection()->get_selected_rows();
    update_from_pool_button->set_sensitive(false);
    for (const auto &path : rows) {
        Gtk::TreeModel::Row row = *item_store->get_iter(path);
        if (row[tree_columns.state] == PoolCacheStatus::Item::State::OUT_OF_DATE) {
            update_from_pool_button->set_sensitive(true);
        }
    }

    if (rows.size() == 1) {
        Gtk::TreeModel::Row row = *item_store->get_iter(rows.at(0));
        PoolCacheStatus::Item::State state = row[tree_columns.state];
        ObjectType type = row[tree_columns.type];
        update_from_pool_button->set_sensitive(state == PoolCacheStatus::Item::State::OUT_OF_DATE);
        if (state == PoolCacheStatus::Item::State::CURRENT) {
            stack->set_visible_child("up_to_date");
        }
        else if (state == PoolCacheStatus::Item::State::MISSING_IN_POOL) {
            stack->set_visible_child("missing_in_pool");
        }
        else if (state == PoolCacheStatus::Item::State::OUT_OF_DATE && type == ObjectType::MODEL_3D) {
            stack->set_visible_child("3d_changed");
        }
        else if (state == PoolCacheStatus::Item::State::OUT_OF_DATE) {
            stack->set_visible_child("delta");
            const json &j = row.get_value(tree_columns.delta);
            std::stringstream ss;
            ss << std::setw(4) << j;
            delta_text_view->get_buffer()->set_text(ss.str());
        }
    }
}

PoolCacheWindow::PoolCacheWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, const std::string &cp,
                                 const std::string &bp, PoolProjectManagerAppWindow *aw)
    : Gtk::Window(cobject), cache_path(cp), base_path(bp), pool(bp), appwin(aw)
{
    x->get_widget("pool_item_view", pool_item_view);
    x->get_widget("stack", stack);
    x->get_widget("delta_text_view", delta_text_view);
    x->get_widget("update_from_pool_button", update_from_pool_button);
    x->get_widget("status_label", status_label);
    update_from_pool_button->set_sensitive(false);
    update_from_pool_button->signal_clicked().connect(sigc::mem_fun(*this, &PoolCacheWindow::update_from_pool));

    Gtk::Button *refresh_list_button;
    x->get_widget("refresh_list_button", refresh_list_button);
    refresh_list_button->signal_clicked().connect([this] { appwin->update_pool_cache_status_now(); });

    Gtk::Button *cleanup_button;
    x->get_widget("cleanup_button", cleanup_button);
    cleanup_button->signal_clicked().connect([this] { appwin->cleanup_pool_cache(); });

    item_store = Gtk::ListStore::create(tree_columns);
    pool_item_view->set_model(item_store);
    pool_item_view->get_selection()->signal_changed().connect(
            sigc::mem_fun(*this, &PoolCacheWindow::selection_changed));
    item_store->set_sort_func(tree_columns.type,
                              [this](const Gtk::TreeModel::iterator &a, const Gtk::TreeModel::iterator &b) {
                                  Gtk::TreeModel::Row ra = *a;
                                  Gtk::TreeModel::Row rb = *b;
                                  ObjectType ta = ra[tree_columns.type];
                                  ObjectType tb = rb[tree_columns.type];
                                  return static_cast<int>(ta) - static_cast<int>(tb);
                              });
    item_store->set_sort_func(tree_columns.state,
                              [this](const Gtk::TreeModel::iterator &a, const Gtk::TreeModel::iterator &b) {
                                  Gtk::TreeModel::Row ra = *a;
                                  Gtk::TreeModel::Row rb = *b;
                                  PoolCacheStatus::Item::State sa = ra[tree_columns.state];
                                  PoolCacheStatus::Item::State sb = rb[tree_columns.state];
                                  return static_cast<int>(sb) - static_cast<int>(sa);
                              });

    {
        auto cr = Gtk::manage(new Gtk::CellRendererText());
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Type", *cr));
        tvc->set_cell_data_func(*cr, [this](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
            Gtk::TreeModel::Row row = *it;
            auto mcr = dynamic_cast<Gtk::CellRendererText *>(tcr);
            mcr->property_text() = object_descriptions.at(row[tree_columns.type]).name;
        });
        tvc->set_sort_column(tree_columns.type);
        pool_item_view->append_column(*tvc);
    }

    {
        auto col = tree_view_append_column_ellipsis(pool_item_view, "Name", tree_columns.name, Pango::ELLIPSIZE_END);
        col->set_min_width(200);
        col->set_sort_column(tree_columns.name);
        col->set_expand(true);
    }

    {
        auto cr = Gtk::manage(new CellRendererColorBox());
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("State"));
        tvc->pack_start(*cr, false);
        auto cr2 = Gtk::manage(new Gtk::CellRendererText());
        cr2->property_text() = "hallo";

        tvc->set_cell_data_func(*cr2, [this](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
            Gtk::TreeModel::Row row = *it;
            auto mcr = dynamic_cast<Gtk::CellRendererText *>(tcr);
            switch (row[tree_columns.state]) {
            case PoolCacheStatus::Item::State::CURRENT:
                mcr->property_text() = "Current";
                break;

            case PoolCacheStatus::Item::State::OUT_OF_DATE:
                mcr->property_text() = "Out of date";
                break;

            case PoolCacheStatus::Item::State::MISSING_IN_POOL:
                mcr->property_text() = "Missing in pool";
                break;
            }
        });
        tvc->set_cell_data_func(*cr, [this](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
            Gtk::TreeModel::Row row = *it;
            auto mcr = dynamic_cast<CellRendererColorBox *>(tcr);
            Color co(1, 0, 1);
            switch (row[tree_columns.state]) {
            case PoolCacheStatus::Item::State::CURRENT:
                co = Color::new_from_int(138, 226, 52);
                break;

            case PoolCacheStatus::Item::State::OUT_OF_DATE:
                co = Color::new_from_int(252, 175, 62);
                break;

            case PoolCacheStatus::Item::State::MISSING_IN_POOL:
                co = Color::new_from_int(239, 41, 41);
                break;
            }
            Gdk::RGBA va;
            va.set_red(co.r);
            va.set_green(co.g);
            va.set_blue(co.b);
            mcr->property_color() = va;
        });
        tvc->pack_start(*cr2, false);
        tvc->set_sort_column(tree_columns.state);
        pool_item_view->append_column(*tvc);
    }

    item_store->set_sort_column(tree_columns.state, Gtk::SORT_ASCENDING);
}

void PoolCacheWindow::select_items(const ItemSet &items)
{
    pool_item_view->get_selection()->unselect_all();
    for (const auto &it : item_store->children()) {
        Gtk::TreeModel::Row row = *it;
        std::pair<ObjectType, UUID> item(row[tree_columns.type], row[tree_columns.uuid]);
        if (items.count(item))
            pool_item_view->get_selection()->select(it);
    }
}

} // namespace horizon
