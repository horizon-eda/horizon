#include "pool_cache_box.hpp"
#include "pool/project_pool.hpp"
#include "pool/pool_info.hpp"
#include "pool/pool_cache_status.hpp"
#include "widgets/cell_renderer_color_box.hpp"
#include "common/object_descr.hpp"
#include "util/gtk_util.hpp"
#include "util/util.hpp"
#include <iomanip>

namespace horizon {

PoolCacheBox::PoolCacheBox(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, IPool &p, Mode m,
                           const std::set<std::string> &items_mod)
    : Gtk::Box(cobject), mode(m), pool(p), items_modified(items_mod)
{
    x->get_widget("pool_item_view", pool_item_view);
    x->get_widget("stack", stack);
    x->get_widget("delta_text_view", delta_text_view);
    x->get_widget("update_from_pool_button", update_from_pool_button);
    x->get_widget("status_label", status_label);
    update_from_pool_button->set_sensitive(false);
    update_from_pool_button->signal_clicked().connect(sigc::mem_fun(*this, &PoolCacheBox::update_from_pool));

    Gtk::Button *refresh_list_button;
    x->get_widget("refresh_list_button", refresh_list_button);
    refresh_list_button->signal_clicked().connect(sigc::mem_fun(*this, &PoolCacheBox::refresh_status));

    Gtk::Button *cleanup_button;
    x->get_widget("cleanup_button", cleanup_button);
    cleanup_button->signal_clicked().connect(sigc::mem_fun(*this, &PoolCacheBox::cleanup));

    item_store = Gtk::ListStore::create(tree_columns);
    pool_item_view->set_model(item_store);
    pool_item_view->get_selection()->signal_changed().connect(sigc::mem_fun(*this, &PoolCacheBox::selection_changed));
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

        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Type"));
        Gtk::CellRendererToggle *cr_toggle = nullptr;
        if (mode == Mode::IMP) {
            cr_toggle = Gtk::manage(new Gtk::CellRendererToggle());
            cr_toggle->signal_toggled().connect([this](const Glib::ustring &path) {
                auto it = item_store->get_iter(path);
                if (it) {
                    Gtk::TreeModel::Row row = *it;
                    row[tree_columns.checked] = !row[tree_columns.checked];
                }
            });
            tvc->pack_start(*cr_toggle);
        }
        {
            auto cr = Gtk::manage(new Gtk::CellRendererText());
            tvc->pack_start(*cr);
            tvc->set_cell_data_func(*cr, [this](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
                Gtk::TreeModel::Row row = *it;
                auto mcr = dynamic_cast<Gtk::CellRendererText *>(tcr);
                mcr->property_text() = object_descriptions.at(row[tree_columns.type]).name;
            });
        }
        tvc->set_sort_column(tree_columns.type);
        auto col = pool_item_view->get_column(pool_item_view->append_column(*tvc) - 1);
        if (cr_toggle)
            col->add_attribute(cr_toggle->property_active(), tree_columns.checked);
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
            va.set_alpha(1);
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

    pool_item_view->signal_row_activated().connect(
            [this](const Gtk::TreeModel::Path &path, Gtk::TreeViewColumn *column) {
                auto it = item_store->get_iter(path);
                if (it) {
                    Gtk::TreeModel::Row row = *it;
                    const auto &item = row.get_value(tree_columns.item);
                    if (item.type != ObjectType::MODEL_3D)
                        s_signal_goto.emit(item.type, item.uuid);
                }
            });

    if (mode == Mode::IMP) {
        refresh_status();
        Gtk::Box *top_box;
        Gtk::Separator *separator;
        x->get_widget("top_box", top_box);
        x->get_widget("separator", separator);
        top_box->hide();
        top_box->set_no_show_all();
        separator->hide();
        separator->set_no_show_all();
    }
}

void PoolCacheBox::refresh_status()
{
    auto status = PoolCacheStatus::from_project_pool(pool);

    refresh_list(status);
}

void PoolCacheBox::selection_changed()
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
            const json &j = row.get_value(tree_columns.item).delta;
            std::stringstream ss;
            ss << std::setw(4) << j;
            delta_text_view->get_buffer()->set_text(ss.str());
        }
    }
}
void PoolCacheBox::update_from_pool()
{
    std::vector<std::string> filenames;
    auto rows = pool_item_view->get_selection()->get_selected_rows();

    for (const auto &path : rows) {
        Gtk::TreeModel::Row row = *item_store->get_iter(path);
        const auto &item = row.get_value(tree_columns.item);
        update_item(item, filenames);
    }


    s_signal_pool_update.emit(filenames);
}

std::vector<std::string> PoolCacheBox::update_checked()
{
    std::vector<std::string> filenames;

    for (auto &row : item_store->children()) {
        if (row.get_value(tree_columns.checked)) {
            const auto &item = row.get_value(tree_columns.item);
            update_item(item, filenames);
        }
    }
    return filenames;
}

void PoolCacheBox::update_item(const PoolCacheStatus::Item &item, std::vector<std::string> &filenames)
{
    const auto state = item.state;
    const auto type = item.type;
    if (state == PoolCacheStatus::Item::State::OUT_OF_DATE) {
        if (type == ObjectType::PACKAGE) {
            json j = load_json_from_file(item.filename_pool);
            ProjectPool::patch_package(j, item.pool_uuid);
            save_json_to_file(item.filename_cached, j);
        }
        else {
            auto dst = Gio::File::create_for_path(item.filename_cached);
            auto src = Gio::File::create_for_path(item.filename_pool);
            src->copy(dst, Gio::FILE_COPY_OVERWRITE);
        }
        if (type != ObjectType::MODEL_3D) {
            filenames.push_back(item.filename_cached);
        }
    }
}

void PoolCacheBox::refresh_list(const PoolCacheStatus &status)
{
    std::set<UUID> selected_uuids;
    {
        auto rows = pool_item_view->get_selection()->get_selected_rows();
        for (const auto &path : rows) {
            Gtk::TreeModel::Row row = *item_store->get_iter(path);
            selected_uuids.insert(row.get_value(tree_columns.item).uuid);
        }
    }

    item_store->freeze_notify();
    item_store->clear();
    for (const auto &it : status.items) {
        if (mode == Mode::IMP && it.state != PoolCacheStatus::Item::State::OUT_OF_DATE)
            continue;
        Gtk::TreeModel::Row row = *item_store->append();
        row[tree_columns.name] = it.name;
        row[tree_columns.type] = it.type;
        row[tree_columns.state] = it.state;
        row[tree_columns.item] = it;
        row[tree_columns.checked] = items_modified.count(it.filename_pool);
    }
    item_store->thaw_notify();
    for (const auto &it : item_store->children()) {
        Gtk::TreeModel::Row row = *it;
        if (selected_uuids.count(row.get_value(tree_columns.item).uuid)) {
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

void PoolCacheBox::cleanup()
{
    s_signal_cleanup_cache.emit();
}

PoolCacheBox *PoolCacheBox::create(IPool &p, Mode mode, const std::set<std::string> &items_modified)
{
    PoolCacheBox *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/horizon-eda/horizon/widgets/pool_cache_box.ui");
    x->get_widget_derived("box", w, p, mode, items_modified);
    w->reference();
    return w;
}

PoolCacheBox *PoolCacheBox::create_for_imp(IPool &p, const std::set<std::string> &items_modified)
{
    return create(p, Mode::IMP, items_modified);
}

PoolCacheBox *PoolCacheBox::create_for_pool_notebook(IPool &p)
{
    return create(p, Mode::POOL_NOTEBOOK, {});
}

} // namespace horizon
