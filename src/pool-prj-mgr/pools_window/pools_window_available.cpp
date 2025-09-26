#include "pools_window.hpp"
#include "common/lut.hpp"
#include "util/util.hpp"
#include <nlohmann/json.hpp>
#include "pool/pool_manager.hpp"
#include "pool_download_window.hpp"
#include "pool-prj-mgr/pool-prj-mgr-app.hpp"
#include "util/http_client.hpp"

namespace horizon {
using json = nlohmann::json;

enum class PoolState { INSTALLED, DISABLED, NOT_INSTALLED };

static PoolState get_pool_state(const UUID &uu)
{
    const PoolManagerPool *pool = nullptr;
    for (const auto &[bp, it] : PoolManager::get().get_pools()) {
        if (it.uuid == uu) {
            pool = &it;
            if (it.enabled)
                break;
        }
    }
    if (pool) {
        if (pool->enabled)
            return PoolState::INSTALLED;
        else
            return PoolState::DISABLED;
    }
    else {
        return PoolState::NOT_INSTALLED;
    }
}

class AvailablePoolItemEditor : public Gtk::Box {
public:
    AvailablePoolItemEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, const PoolIndex &aidx)
        : Gtk::Box(cobject), pool_index(aidx)
    {
        x->get_widget("available_pool_item_name", label_name);
        x->get_widget("available_pool_item_level", label_level);
        x->get_widget("available_pool_item_source", label_source);
        x->get_widget("available_pool_item_state", label_state);
        x->get_widget("available_pool_item_download", download_button);
        label_name->set_text(pool_index.name);
        label_level->set_text("(" + PoolIndex::level_lut.lookup_reverse(pool_index.level) + ")");
        label_source->set_markup("GitHub: <a href=\"https://github.com/" + pool_index.gh_user + "/" + pool_index.gh_repo
                                 + "\">" + pool_index.gh_user + "/" + pool_index.gh_repo + "</a>");
        const auto pool_state = get_pool_state(pool_index.uuid);
        switch (pool_state) {
        case PoolState::INSTALLED:
            label_state->set_text("installed");
            download_button->set_sensitive(false);
            break;
        case PoolState::DISABLED:
            label_state->set_text("installed, not enabled");
            download_button->set_sensitive(false);
            break;
        case PoolState::NOT_INSTALLED: {
            std::string l = "not installed";
            bool can_download = true;
            for (const auto &uu : pool_index.included) {
                const auto st = get_pool_state(uu);
                if (st == PoolState::DISABLED) {
                    l += "\nDependency not enabled";
                    can_download = false;
                }
                else if (st == PoolState::NOT_INSTALLED) {
                    l += "\nDependency not installed";
                    can_download = false;
                }
            }
            download_button->set_sensitive(can_download);
            label_state->set_text(l);
        } break;
        }
    }
    static AvailablePoolItemEditor *create(const PoolIndex &idx)
    {
        AvailablePoolItemEditor *w;
        Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
        std::vector<Glib::ustring> objs = {"available_pool_item_box"};
        x->add_from_resource("/org/horizon-eda/horizon/pool-prj-mgr/pools_window/pools_window.ui", objs);
        x->get_widget_derived("available_pool_item_box", w, idx);

        w->reference();
        return w;
    }
    const PoolIndex pool_index;
    Gtk::Button *download_button;

private:
    Gtk::Label *label_name;
    Gtk::Label *label_level;
    Gtk::Label *label_source;
    Gtk::Label *label_state;
};

void PoolsWindow::update_index()
{
    if (index_fetch_thread.joinable())
        return;
    available_placeholder_label->set_text("Loading…");
    index_fetch_thread = std::thread(&PoolsWindow::index_fetch_worker, this);
}

void PoolsWindow::index_fetch_worker()
{
    std::string err;
    try {
        HTTP::Client client;
        const auto s = client.get("https://pool-index.horizon-eda.org/index.json");
        const auto j = json::parse(s);
        const auto version = j.at("version").get<int>();
        if (version > 1) {
            throw std::runtime_error("unsupported version " + std::to_string(version) + "\nTry updating Horizon EDA.");
        }
        {
            std::lock_guard<std::mutex> lock(index_mutex);
            pools_index_thread.clear();
            for (const auto &[k, v] : j.at("pools").items()) {
                const auto uu = UUID(k);
                pools_index_thread.emplace(std::piecewise_construct, std::forward_as_tuple(uu),
                                           std::forward_as_tuple(uu, v));
            }
        }
    }
    catch (const std::exception &e) {
        err = e.what();
    }
    if (err.size()) {
        std::lock_guard<std::mutex> lock(index_mutex);
        pools_index_err_thread = err;
    }
    index_dispatcher.emit();
}

static bool operator<(PoolIndex::Level a, PoolIndex::Level b)
{
    return static_cast<int>(a) < static_cast<int>(b);
}

void PoolsWindow::update_available()
{
    auto children = available_listbox->get_children();
    for (auto it : children) {
        delete it;
    }
    std::vector<const PoolIndex *> pools_index_sorted;
    pools_index_sorted.reserve(pools_index.size());
    for (const auto &[uu, it] : pools_index) {
        pools_index_sorted.push_back(&it);
    }
    std::sort(pools_index_sorted.begin(), pools_index_sorted.end(), [](auto a, auto b) { return a->level < b->level; });
    for (const auto it : pools_index_sorted) {
        auto x = AvailablePoolItemEditor::create(*it);
        auto row = Gtk::manage(new Gtk::ListBoxRow);
        row->add(*x);
        row->show_all();
        available_listbox->add(*row);
        row->set_activatable(false);
        x->unreference();
        x->download_button->signal_clicked().connect([this, x] { show_download_window(&x->pool_index); });
    }
}

void PoolsWindow::show_download_window(const PoolIndex *idx)
{
    auto win = PoolDownloadWindow::create(idx);
    win->set_transient_for(*this);
    win->present();
    win->signal_hide().connect([win] { delete win; });
    win->signal_downloaded().connect(sigc::track_obj(
            [this](auto filename) {
                stack->set_visible_child("installed");
                app.add_recent_item(filename);
                update();
            },
            *this));
}

} // namespace horizon
