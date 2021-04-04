#include "pools_window.hpp"
#include "util/gtk_util.hpp"
#include "util/util.hpp"
#include "pool/pool_manager.hpp"
#include <thread>
#include <filesystem>
#include "util/changeable.hpp"
#include "common/common.hpp"
#include "nlohmann/json.hpp"
#include "pool/pool.hpp"
#include "common/object_descr.hpp"
#include "pool_status_provider.hpp"
#include "pool_merge_box.hpp"
#include "../pool-prj-mgr-app.hpp"

namespace horizon {
namespace fs = std::filesystem;
using json = nlohmann::json;


class PoolItemEditor : public Gtk::Box {
public:
    PoolItemEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, const PoolManagerPool &pool,
                   PoolStatusProviderBase *aprv)
        : Gtk::Box(cobject), base_path(pool.base_path), prv(aprv)
    {
        x->get_widget("pool_item_name", label_name);
        x->get_widget("pool_item_path", label_path);
        x->get_widget("pool_item_uuid", label_uuid);
        x->get_widget("pool_item_delete", button_delete);
        x->get_widget("pool_item_enabled", switch_enabled);
        x->get_widget("pool_item_managed", label_managed);
        label_name->set_text(pool.name);
        label_path->set_text(pool.base_path);
        label_uuid->set_text((std::string)pool.uuid);
        switch_enabled->set_active(pool.enabled);
        auto bp = fs::u8path(pool.base_path);

        if (prv) {
            prv->signal_changed().connect(sigc::track_obj([this] { update_from_prv(); }, *this));
            update_from_prv();
        }
        else {
            label_managed->set_text("Not managed");
        }
    }
    static PoolItemEditor *create(const PoolManagerPool &pool, PoolStatusProviderBase *prv)
    {
        PoolItemEditor *w;
        Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
        std::vector<Glib::ustring> objs = {"pool_item_box", "image1"};
        x->add_from_resource("/org/horizon-eda/horizon/pool-prj-mgr/pools_window/pools_window.ui", objs);
        x->get_widget_derived("pool_item_box", w, pool, prv);

        w->reference();
        return w;
    }
    Gtk::Button *button_delete;
    Gtk::Switch *switch_enabled;

    const std::string base_path;

private:
    PoolStatusProviderBase *prv;
    enum class Manager { REMOTE, GIT, NONE };
    Manager manager = Manager::NONE;
    Gtk::Label *label_managed;
    Gtk::Label *label_name;
    Gtk::Label *label_path;
    Gtk::Label *label_uuid;
    void update_from_prv()
    {
        if (!prv)
            return;
        auto &st = prv->get();
        label_managed->set_text(st.get_brief());
    }
};

class PoolInfoBox : public Gtk::Box {
public:
    PoolInfoBox(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, const PoolManagerPool &apool,
                PoolStatusProviderBase *aprv, PoolProjectManagerApplication &a_app)
        : Gtk::Box(cobject), base_path(apool.base_path), prv(aprv), app(a_app)
    {
        Gtk::Label *pool_info_name_label;
        Gtk::Label *pool_info_stats_label;
        GET_WIDGET(pool_info_name_label);
        GET_WIDGET(pool_info_stats_label);
        pool_info_name_label->set_text(apool.name);
        Pool pool(apool.base_path);
        std::string stats;
        SQLite::Query q(pool.db, "SELECT type, COUNT(*) FROM all_items_view WHERE pool_uuid = ? GROUP BY type");
        q.bind(1, apool.uuid);
        while (q.step()) {
            const auto type = q.get<ObjectType>(0);
            const auto count = q.get<int>(1);
            if (stats.size())
                stats += ", ";
            stats += std::to_string(count) + " " + object_descriptions.at(type).get_name_for_n(count);
        }
        pool_info_stats_label->set_text(stats);

        Gtk::Box *pool_info_delta_box;
        GET_WIDGET(pool_info_delta_box);
        if (auto p = dynamic_cast<PoolStatusProviderPoolManager *>(prv)) {
            auto box = PoolMergeBox2::create(*p);
            pool_info_delta_box->pack_start(*box, true, true, 0);
            box->show();
            box->unreference();
        }
        {
            Gtk::Button *pool_info_open_button;
            GET_WIDGET(pool_info_open_button);
            pool_info_open_button->signal_clicked().connect(
                    [this] { app.open_pool((fs::u8path(base_path) / "pool.json").u8string()); });
        }
    }
    static PoolInfoBox *create(const PoolManagerPool &pool, PoolStatusProviderBase *prv,
                               PoolProjectManagerApplication &app)
    {
        PoolInfoBox *w;
        Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
        std::vector<Glib::ustring> objs = {"pool_info_box"};
        x->add_from_resource("/org/horizon-eda/horizon/pool-prj-mgr/pools_window/pools_window.ui", objs);
        x->get_widget_derived("pool_info_box", w, pool, prv, app);

        w->reference();
        return w;
    }


private:
    const std::string base_path;
    PoolStatusProviderBase *prv;
    PoolProjectManagerApplication &app;
};

PoolsWindow *PoolsWindow::create(class PoolProjectManagerApplication &a_app)
{
    PoolsWindow *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    std::vector<Glib::ustring> objs = {"window", "image2"};
    x->add_from_resource("/org/horizon-eda/horizon/pool-prj-mgr/pools_window/pools_window.ui", objs);
    x->get_widget_derived("window", w, a_app);

    return w;
}

PoolsWindow::PoolsWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x,
                         class PoolProjectManagerApplication &a_app)
    : Gtk::Window(cobject), mgr(PoolManager::get()), app(a_app)
{
    GET_WIDGET(installed_listbox);
    GET_WIDGET(info_box);
    GET_WIDGET(available_listbox);
    GET_WIDGET(stack);
    GET_WIDGET(button_stack);
    GET_WIDGET(available_placeholder_label);
    installed_listbox->set_header_func(&header_func_separator);
    available_listbox->set_header_func(&header_func_separator);
    stack->property_visible_child_name().signal_changed().connect(
            [this] { button_stack->set_visible_child(stack->get_visible_child_name()); });
    {
        Gtk::Button *button_add_pool;
        GET_WIDGET(button_add_pool);
        button_add_pool->signal_clicked().connect([this] { add_pool(""); });
    }
    {
        Gtk::Button *button_check_for_updates;
        GET_WIDGET(button_check_for_updates);
        button_check_for_updates->signal_clicked().connect([this] {
            for (auto &[bp, prv] : pool_status_providers) {
                prv->check_for_updates();
            }
        });
    }
    installed_listbox->signal_selected_rows_changed().connect([this] {
        delete pool_info_box;
        pool_info_box = nullptr;
        if (auto row = installed_listbox->get_selected_row()) {
            if (auto ch = dynamic_cast<PoolItemEditor *>(row->get_child())) {
                PoolStatusProviderBase *prv = nullptr;
                if (pool_status_providers.count(ch->base_path)) {
                    prv = pool_status_providers.at(ch->base_path).get();
                }
                pool_info_box = PoolInfoBox::create(PoolManager::get().get_pools().at(ch->base_path), prv, app);
                info_box->pack_start(*pool_info_box, true, true, 0);
                pool_info_box->show();
                pool_info_box->unreference();
            }
        }
    });
    update();

    index_dispatcher.connect([this] {
        std::string err;
        {
            std::lock_guard<std::mutex> lock(index_mutex);
            pools_index = pools_index_thread;
            err = pools_index_err_thread;
        }
        if (index_fetch_thread.joinable())
            index_fetch_thread.join();
        if (err.size())
            available_placeholder_label->set_text(err);
        else
            update_available();
    });
    update_index();

    if (PoolManager::get().get_pools().size() == 0) {
        stack->set_visible_child("available");
    }

    {
        Gtk::Button *download_unlisted_button;
        GET_WIDGET(download_unlisted_button);
        download_unlisted_button->signal_clicked().connect([this] { show_download_window(nullptr); });
    }
    {
        Gtk::Button *refresh_available_button;
        GET_WIDGET(refresh_available_button);
        refresh_available_button->signal_clicked().connect([this] { update_index(); });
    }
}


void PoolsWindow::update()
{
    std::optional<std::string> pool_sel;
    if (auto row = installed_listbox->get_selected_row()) {
        if (auto w = dynamic_cast<PoolItemEditor *>(row->get_child())) {
            pool_sel = w->base_path;
        }
    }
    auto children = installed_listbox->get_children();
    for (auto it : children) {
        delete it;
    }
    const auto pools = mgr.get_pools();
    for (const auto &it : pools) {
        PoolStatusProviderBase *prv = nullptr;
        if (pool_status_providers.count(it.first)) {
            prv = pool_status_providers.at(it.first).get();
        }
        else {
            auto bp = fs::u8path(it.first);
            if (fs::is_directory(bp / ".remote")) {
                prv = pool_status_providers.emplace(it.first, std::make_unique<PoolStatusProviderPoolManager>(it.first))
                              .first->second.get();
                prv->refresh();
            }
        }
        auto x = PoolItemEditor::create(it.second, prv);
        auto row = Gtk::manage(new Gtk::ListBoxRow);
        row->add(*x);
        row->show_all();
        installed_listbox->add(*row);
        row->set_activatable(false);
        x->unreference();
        std::string bp = it.first;
        x->button_delete->signal_clicked().connect([bp, this] {
            mgr.remove_pool(bp);
            update();
        });
        x->switch_enabled->property_active().signal_changed().connect([bp, this, x] {
            mgr.set_pool_enabled(bp, x->switch_enabled->get_active());
            update();
        });
        if (pool_sel && *pool_sel == it.first) {
            installed_listbox->select_row(*row);
        }
    }
    map_erase_if(pool_status_providers, [&pools](const auto &x) { return pools.count(x.first) == 0; });
    update_available();
}

void PoolsWindow::add_pool(const std::string &path)
{
    GtkFileChooserNative *native =
            gtk_file_chooser_native_new("Add Pool", gobj(), GTK_FILE_CHOOSER_ACTION_OPEN, "_Open", "_Cancel");
    auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
    auto filter = Gtk::FileFilter::create();
    filter->set_name("Horizon pool (pool.json)");
    filter->add_pattern("pool.json");
    chooser->add_filter(filter);
    if (path.size())
        chooser->set_filename(path);

    if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(native)) == GTK_RESPONSE_ACCEPT) {
        auto cpath = chooser->get_file()->get_parent()->get_path();
        mgr.add_pool(cpath);
        update();
    }
}

PoolsWindow::~PoolsWindow()
{
    if (index_fetch_thread.joinable())
        index_fetch_thread.join();
}

} // namespace horizon
