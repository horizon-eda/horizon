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

namespace horizon {
PoolCacheWindow *PoolCacheWindow::create(Gtk::Window *p, const std::string &cache_path, const std::string &pool_path,
                                         PoolProjectManagerAppWindow *aw)
{
    PoolCacheWindow *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/net/carrotIndustries/horizon/pool-prj-mgr/prj-mgr/pool_cache_window.ui");
    x->get_widget_derived("window", w, cache_path, pool_path, aw);

    w->set_transient_for(*p);

    return w;
}

static void find_files(const std::string &base_path, std::function<void(const std::string &)> cb,
                       const std::string &path = "")
{
    auto this_path = Glib::build_filename(base_path, path);
    Glib::Dir dir(this_path);
    for (const auto &it : dir) {
        auto itempath = Glib::build_filename(this_path, it);
        if (Glib::file_test(itempath, Glib::FILE_TEST_IS_DIR)) {
            find_files(base_path, cb, Glib::build_filename(path, it));
        }
        else if (Glib::file_test(itempath, Glib::FILE_TEST_IS_REGULAR)) {
            cb(Glib::build_filename(path, it));
        }
    }
}

void PoolCacheWindow::refresh_list()
{
    pool_cached.clear();
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
    Glib::Dir dir(cache_path);
    unsigned int n_total = 0;
    unsigned int n_current = 0;
    unsigned int n_missing = 0;
    unsigned int n_out_of_date = 0;
    for (const auto &it : dir) {
        if (endswith(it, ".json")) {
            auto itempath = Glib::build_filename(cache_path, it);
            json j_cache;
            {
                std::ifstream ifs(itempath);
                if (!ifs.is_open()) {
                    throw std::runtime_error("file " + itempath + " not opened");
                }
                ifs >> j_cache;
                if (j_cache.count("_imp"))
                    j_cache.erase("_imp");

                ifs.close();
            }
            Gtk::TreeModel::Row row;
            row = *item_store->append();
            n_total++;
            std::string type_str = j_cache.at("type");
            row[tree_columns.filename_cached] = itempath;
            if (type_str == "part") {
                try {
                    auto part = Part::new_from_file(itempath, pool_cached);
                    row[tree_columns.name] = part.get_MPN() + " / " + part.get_manufacturer();
                }
                catch (...) {
                    row[tree_columns.name] = "Part not found";
                }
            }
            else {
                row[tree_columns.name] = j_cache.at("name").get<std::string>();
            }
            ObjectType type = object_type_lut.lookup(type_str);
            row[tree_columns.type] = type;

            UUID uuid(j_cache.at("uuid").get<std::string>());
            row[tree_columns.uuid] = uuid;

            std::string pool_filename;
            try {
                pool_filename = pool.get_filename(type, uuid);
            }
            catch (...) {
                pool_filename = "";
            }
            if (pool_filename.size() && Glib::file_test(pool_filename, Glib::FILE_TEST_IS_REGULAR)) {
                json j_pool;
                {
                    std::ifstream ifs(pool_filename);
                    if (!ifs.is_open()) {
                        throw std::runtime_error("file " + pool_filename + " not opened");
                    }
                    ifs >> j_pool;
                    if (j_pool.count("_imp"))
                        j_pool.erase("_imp");
                    ifs.close();
                }
                auto delta = json::diff(j_cache, j_pool);
                row[tree_columns.delta] = delta;
                if (delta.size()) {
                    row[tree_columns.state] = ItemState::OUT_OF_DATE;
                    n_out_of_date++;
                }
                else {
                    row[tree_columns.state] = ItemState::CURRENT;
                    n_current++;
                }
            }
            else {
                row[tree_columns.state] = ItemState::MISSING_IN_POOL;
                n_missing++;
            }
        }
    }
    {
        auto models_path = Glib::build_filename(cache_path, "3d_models");
        if (Glib::file_test(models_path, Glib::FILE_TEST_IS_DIR)) {
            Glib::Dir dir_3d(models_path);
            for (const auto &it : dir_3d) {
                if (it.find("pool_") == 0 && it.size() == 41) {
                    UUID pool_uuid(it.substr(5));
                    auto it_pool = PoolManager::get().get_by_uuid(pool_uuid);
                    if (it_pool) {
                        auto &pool_base_path = it_pool->base_path;
                        auto pool_cache_path = Glib::build_filename(models_path, it);
                        find_files(pool_cache_path, [this, &pool_cache_path, &n_total, &n_missing, &n_current,
                                                     &n_out_of_date, &pool_base_path](const std::string &path) {
                            Gtk::TreeModel::Row row;
                            row = *item_store->append();
                            n_total++;
                            auto filename_cached = Glib::build_filename(pool_cache_path, path);
                            auto filename_pool = Glib::build_filename(pool_base_path, path);
                            row[tree_columns.filename_cached] = path;
                            row[tree_columns.name] = Glib::path_get_basename(path);
                            row[tree_columns.type] = ObjectType::MODEL_3D;
                            row[tree_columns.uuid] = UUID::random();
                            if (Glib::file_test(filename_pool, Glib::FILE_TEST_IS_REGULAR)) {
                                if (compare_files(filename_cached, filename_pool)) {
                                    row[tree_columns.state] = ItemState::CURRENT;
                                    n_current++;
                                }
                                else {
                                    row[tree_columns.state] = ItemState::OUT_OF_DATE;
                                    n_out_of_date++;
                                }
                            }
                            else {
                                row[tree_columns.state] = ItemState::MISSING_IN_POOL;
                                n_missing++;
                            }
                        });
                    }
                }
            }
        }
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
    ss << "Total:" << n_total << " ";
    ss << "Current:" << n_current << " ";
    if (n_out_of_date)
        ss << "Out of date:" << n_out_of_date << " ";
    if (n_missing)
        ss << "Missing:" << n_missing;

    status_label->set_text(ss.str());
}

void PoolCacheWindow::update_from_pool()
{
    auto rows = pool_item_view->get_selection()->get_selected_rows();
    for (const auto &path : rows) {
        Gtk::TreeModel::Row row = *item_store->get_iter(path);
        ItemState state = row[tree_columns.state];
        ObjectType type = row[tree_columns.type];
        if (state == ItemState::OUT_OF_DATE && type == ObjectType::MODEL_3D) {
            std::string model_path = Glib::build_filename("3d_models", row[tree_columns.filename_cached]);
            auto dst = Gio::File::create_for_path(Glib::build_filename(cache_path, model_path));
            auto src = Gio::File::create_for_path(Glib::build_filename(pool.get_base_path(), model_path));
            src->copy(dst, Gio::FILE_COPY_OVERWRITE);
        }
        else if (state == ItemState::OUT_OF_DATE) {
            auto dst = Gio::File::create_for_path(row[tree_columns.filename_cached]);
            auto src = Gio::File::create_for_path(pool.get_filename(row[tree_columns.type], row[tree_columns.uuid]));
            src->copy(dst, Gio::FILE_COPY_OVERWRITE);
        }
    }
    refresh_list();
}

void PoolCacheWindow::selection_changed()
{
    auto rows = pool_item_view->get_selection()->get_selected_rows();
    update_from_pool_button->set_sensitive(false);
    for (const auto &path : rows) {
        Gtk::TreeModel::Row row = *item_store->get_iter(path);
        if (row[tree_columns.state] == ItemState::OUT_OF_DATE) {
            update_from_pool_button->set_sensitive(true);
        }
    }

    if (rows.size() == 1) {
        Gtk::TreeModel::Row row = *item_store->get_iter(rows.at(0));
        ItemState state = row[tree_columns.state];
        ObjectType type = row[tree_columns.type];
        update_from_pool_button->set_sensitive(state == ItemState::OUT_OF_DATE);
        if (state == ItemState::CURRENT) {
            stack->set_visible_child("up_to_date");
        }
        else if (state == ItemState::MISSING_IN_POOL) {
            stack->set_visible_child("missing_in_pool");
        }
        else if (state == ItemState::OUT_OF_DATE && type == ObjectType::MODEL_3D) {
            stack->set_visible_child("3d_changed");
        }
        else if (state == ItemState::OUT_OF_DATE) {
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
    : Gtk::Window(cobject), cache_path(cp), base_path(bp), pool_cached(bp, cp), pool(bp), appwin(aw)
{
    x->get_widget("pool_item_view", pool_item_view);
    x->get_widget("stack", stack);
    x->get_widget("delta_text_view", delta_text_view);
    x->get_widget("update_from_pool_button", update_from_pool_button);
    x->get_widget("status_label", status_label);
    update_from_pool_button->set_sensitive(false);

    Gtk::Button *refresh_list_button;
    x->get_widget("refresh_list_button", refresh_list_button);
    refresh_list_button->signal_clicked().connect(sigc::mem_fun(*this, &PoolCacheWindow::refresh_list));
    update_from_pool_button->signal_clicked().connect(sigc::mem_fun(*this, &PoolCacheWindow::update_from_pool));

    Gtk::Button *cleanup_button;
    x->get_widget("cleanup_button", cleanup_button);
    cleanup_button->signal_clicked().connect([this] {
        appwin->cleanup_pool_cache();
        refresh_list();
    });


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
                                  ItemState sa = ra[tree_columns.state];
                                  ItemState sb = rb[tree_columns.state];
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
        auto col = pool_item_view->append_column("Name", tree_columns.name);
        std::cout << "col " << col << std::endl;
        pool_item_view->get_column(col - 1)->set_sort_column(tree_columns.name);
    }

    {
        auto cr = Gtk::manage(new CellRendererColorBox());
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("State", *cr));
        auto cr2 = Gtk::manage(new Gtk::CellRendererText());
        cr2->property_text() = "hallo";

        tvc->set_cell_data_func(*cr2, [this](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
            Gtk::TreeModel::Row row = *it;
            auto mcr = dynamic_cast<Gtk::CellRendererText *>(tcr);
            switch (row[tree_columns.state]) {
            case ItemState::CURRENT:
                mcr->property_text() = "Current";
                break;

            case ItemState::OUT_OF_DATE:
                mcr->property_text() = "Out of date";
                break;

            case ItemState::MISSING_IN_POOL:
                mcr->property_text() = "Missing in pool";
                break;
            }
        });
        tvc->set_cell_data_func(*cr, [this](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
            Gtk::TreeModel::Row row = *it;
            auto mcr = dynamic_cast<CellRendererColorBox *>(tcr);
            Color co(1, 0, 1);
            switch (row[tree_columns.state]) {
            case ItemState::CURRENT:
                co = Color::new_from_int(138, 226, 52);
                break;

            case ItemState::OUT_OF_DATE:
                co = Color::new_from_int(252, 175, 62);
                break;

            case ItemState::MISSING_IN_POOL:
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

    refresh_list();
    item_store->set_sort_column(tree_columns.state, Gtk::SORT_ASCENDING);
}
} // namespace horizon
