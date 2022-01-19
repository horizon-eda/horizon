#include "pool_settings_box.hpp"
#include "pool_notebook.hpp"
#include "pool/pool_manager.hpp"
#include "pool/ipool.hpp"
#include "pool/pool_info.hpp"
#include "util/sqlite.hpp"
#include "widgets/pool_browser_button.hpp"
#include "widgets/pool_browser_padstack.hpp"
#include "widgets/pool_browser_frame.hpp"

namespace horizon {


class PoolListItem : public Gtk::Box {
public:
    PoolListItem(const UUID &uu, bool available = true);
    const UUID uuid;
    std::string base_path;
    const bool available;
};

PoolSettingsBox::PoolSettingsBox(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, IPool &p)
    : Gtk::Box(cobject), pool(p), pool_info(pool.get_pool_info())
{
    x->get_widget("button_save_pool", save_button);
    x->get_widget("pool_settings_name_entry", entry_name);
    x->get_widget("pool_settings_available_listbox", pools_available_listbox);
    x->get_widget("pool_settings_included_listbox", pools_included_listbox);
    x->get_widget("pool_settings_actually_included_listbox", pools_actually_included_listbox);
    x->get_widget("pool_inc_button", pool_inc_button);
    x->get_widget("pool_excl_button", pool_excl_button);
    x->get_widget("pool_up_button", pool_up_button);
    x->get_widget("pool_down_button", pool_down_button);
    x->get_widget("pool_settings_hint_label", hint_label);
    hint_label->hide();
    entry_name->set_text(pool.get_pool_info().name);
    entry_name->signal_changed().connect([this] {
        pool_info.name = entry_name->get_text();
        set_needs_save();
    });
    save_button->signal_clicked().connect(sigc::mem_fun(*this, &PoolSettingsBox::save));
    save_button->set_sensitive(false);

    pool_inc_button->signal_clicked().connect([this] { inc_excl_pool(true); });
    pool_excl_button->signal_clicked().connect([this] { inc_excl_pool(false); });
    pool_up_button->signal_clicked().connect([this] { pool_up_down(true); });
    pool_down_button->signal_clicked().connect([this] { pool_up_down(false); });

    for (auto lb : {pools_available_listbox, pools_included_listbox, pools_actually_included_listbox}) {
        lb->signal_row_activated().connect([this](Gtk::ListBoxRow *row) {
            auto it = dynamic_cast<PoolListItem *>(row->get_child());
            s_signal_open_pool.emit(it->base_path);
        });
        lb->signal_row_selected().connect([this](auto row) { update_button_sensitivity(); });
    }

    {
        Gtk::Box *pool_misc_box;
        x->get_widget("pool_misc_box", pool_misc_box);
        {
            auto la = Gtk::manage(new Gtk::Label("Default via"));
            la->set_margin_start(10);
            la->get_style_context()->add_class("dim-label");
            pool_misc_box->pack_start(*la, false, false, 0);
        }
        {
            browser_button_via = Gtk::manage(new PoolBrowserButton(ObjectType::PADSTACK, pool));
            browser_button_via->property_selected_uuid() = pool.get_pool_info().default_via;
            browser_button_via->property_selected_uuid().signal_changed().connect([this] {
                pool_info.default_via = browser_button_via->property_selected_uuid();
                set_needs_save();
            });
            auto &br = dynamic_cast<PoolBrowserPadstack &>(browser_button_via->get_browser());
            br.set_padstacks_included({Padstack::Type::VIA});
            pool_misc_box->pack_start(*browser_button_via, true, true, 0);
        }
        {
            auto la = Gtk::manage(new Gtk::Label("Default frame"));
            la->set_margin_start(10);
            la->get_style_context()->add_class("dim-label");
            pool_misc_box->pack_start(*la, false, false, 0);
        }
        {
            browser_button_frame = Gtk::manage(new PoolBrowserButton(ObjectType::FRAME, pool));
            browser_button_frame->property_selected_uuid() = pool.get_pool_info().default_frame;
            browser_button_frame->property_selected_uuid().signal_changed().connect([this] {
                pool_info.default_frame = browser_button_frame->property_selected_uuid();
                set_needs_save();
            });
            pool_misc_box->pack_start(*browser_button_frame, true, true, 0);
        }


        pool_misc_box->show_all();
    }

    saved_version = pool.get_pool_info().version.get_file();

    update_pools();
    update_actual();
}

PoolSettingsBox *PoolSettingsBox::create(IPool &p)
{
    PoolSettingsBox *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    std::vector<Glib::ustring> widgets = {"box_settings", "image1", "image2", "image3", "image4"};
    x->add_from_resource("/org/horizon-eda/horizon/pool-prj-mgr/window.ui", widgets);
    x->get_widget_derived("box_settings", w, p);
    w->reference();
    return w;
}

void PoolSettingsBox::save()
{
    pool_info.save();
    if (PoolManager::get().get_pools().count(pool_info.base_path))
        PoolManager::get().update_pool(pool_info.base_path);
    saved_version = pool_info.get_required_version();
    save_button->set_sensitive(false);
    hint_label->set_markup("Almost there! For the items of the included pool to show up, click <i>Update pool</i>.");
    needs_save = false;
    s_signal_saved.emit();
    s_signal_changed.emit();
}

void PoolSettingsBox::set_needs_save()
{
    const auto &version = pool_info.version;
    if (version.get_file() > version.get_app()) {
        return;
    }
    s_signal_changed.emit();
    needs_save = true;
    save_button->set_sensitive(true);
    hint_label->set_markup("For the items of the included pool to show up, click <i>Save</i> and <i>Update pool</i>.");
    hint_label->show();
}

std::string PoolSettingsBox::get_version_message() const
{
    const auto required_version = pool_info.get_required_version();
    const auto &version = pool_info.version;
    static const std::string suffix = " This only applies to the settings tab. Pool items have different versions.";
    if (version.get_file() > version.get_app())
        return version.get_message(ObjectType::POOL) + suffix;

    else if (required_version > saved_version)
        return "Saving this Pool will update it from version " + std::to_string(saved_version) + " to "
               + std::to_string(required_version) + "." + suffix + " " + FileVersion::learn_more_markup;

    else
        return "";
}

bool PoolSettingsBox::get_needs_save() const
{
    return needs_save;
}

void PoolSettingsBox::pool_updated()
{
    if (!needs_save)
        hint_label->hide();
    update_actual();
}

PoolListItem::PoolListItem(const UUID &uu, bool avail)
    : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 5), uuid(uu), available(avail)
{
    property_margin() = 5;
    auto pool = PoolManager::get().get_by_uuid(uu);
    {
        auto la = Gtk::manage(new Gtk::Label);
        la->set_xalign(0);
        la->set_markup("<b>" + (pool ? pool->name : "Not found") + "</b>");
        pack_start(*la, true, true, 0);
        la->show();
    }
    {
        auto la = Gtk::manage(new Gtk::Label((std::string)uu));
        la->set_xalign(0);
        pack_start(*la, true, true, 0);
        la->show();
    }
    {
        auto la = Gtk::manage(new Gtk::Label(pool ? pool->base_path : "Not found"));
        if (pool)
            base_path = pool->base_path;
        la->set_xalign(0);
        pack_start(*la, true, true, 0);
        la->show();
    }

    if (!avail) {
        set_tooltip_text(
                "Not available since the opened pool is a depedendency or the selected pool needs to be rebuilt.");
        set_sensitive(false);
    }
}

static std::optional<UUID> pool_from_listbox(Gtk::ListBox *lb)
{
    if (auto row = lb->get_selected_row()) {
        auto it = dynamic_cast<PoolListItem *>(row->get_child());
        if (it->available)
            return it->uuid;
        else
            return {};
    }
    else {
        return {};
    }
}

void PoolSettingsBox::inc_excl_pool(bool inc)
{
    Gtk::ListBox *lb;
    if (inc)
        lb = pools_available_listbox;
    else
        lb = pools_included_listbox;

    if (const auto uu = pool_from_listbox(lb)) {
        if (inc) {
            pool_info.pools_included.push_back(*uu);
        }
        else {
            pool_info.pools_included.erase(
                    std::remove(pool_info.pools_included.begin(), pool_info.pools_included.end(), *uu),
                    pool_info.pools_included.end());
        }
        update_pools();
        set_needs_save();
    }
}

void PoolSettingsBox::pool_up_down(bool up)
{
    if (const auto sel = pool_from_listbox(pools_included_listbox)) {
        const auto uu = *sel;
        auto it = std::find(pool_info.pools_included.begin(), pool_info.pools_included.end(), uu);
        if (it == pool_info.pools_included.end())
            throw std::logic_error("pool not found");
        if (up && it != pool_info.pools_included.begin()) {
            std::swap(*it, *(it - 1));
            update_pools();
            set_needs_save();
        }
        else if (!up && (it != (pool_info.pools_included.end() - 1))) {
            std::swap(*it, *(it + 1));
            update_pools();
            set_needs_save();
        }
    }
}

void PoolSettingsBox::update_pools()
{
    {
        const auto sel = pool_from_listbox(pools_available_listbox);
        for (auto &it : pools_available_listbox->get_children()) {
            delete it;
        }
        for (const auto &it : PoolManager::get().get_pools()) {
            if (it.second.enabled && it.second.uuid != pool.get_pool_info().uuid
                && std::count(pool_info.pools_included.begin(), pool_info.pools_included.end(), it.second.uuid) == 0) {
                bool usable = it.second.is_usable();
                if (usable) {
                    Pool other_pool(it.second.base_path);
                    SQLite::Query q(other_pool.db, "SELECT uuid FROM pools_included WHERE uuid = ?");
                    q.bind(1, pool.get_pool_info().uuid);
                    usable = !q.step();
                }
                auto w = Gtk::manage(new PoolListItem(it.second.uuid, usable));
                pools_available_listbox->append(*w);
                w->show();
                if (sel && it.second.uuid == *sel) {
                    pools_available_listbox->select_row(dynamic_cast<Gtk::ListBoxRow &>(*w->get_parent()));
                }
            }
        }
    }
    {
        const auto sel = pool_from_listbox(pools_included_listbox);
        for (auto &it : pools_included_listbox->get_children()) {
            delete it;
        }
        for (const auto &uu : pool_info.pools_included) {
            auto w = Gtk::manage(new PoolListItem(uu));
            pools_included_listbox->append(*w);
            w->show();
            if (sel && uu == *sel) {
                pools_included_listbox->select_row(dynamic_cast<Gtk::ListBoxRow &>(*w->get_parent()));
            }
        }
    }
    update_button_sensitivity();
}

void PoolSettingsBox::update_actual()
{
    for (auto &it : pools_actually_included_listbox->get_children()) {
        delete it;
    }
    SQLite::Query q(pool.get_db(), "SELECT uuid FROM pools_included WHERE level != 0 ORDER BY level ASC");
    while (q.step()) {
        const UUID uu = q.get<std::string>(0);
        auto w = Gtk::manage(new PoolListItem(uu));
        pools_actually_included_listbox->append(*w);
        w->show();
    }
}

void PoolSettingsBox::update_button_sensitivity()
{
    pool_inc_button->set_sensitive(pool_from_listbox(pools_available_listbox).has_value());
    const auto sel = pool_from_listbox(pools_included_listbox);
    pool_excl_button->set_sensitive(sel.has_value());
    if (sel.has_value()) {
        auto it = std::find(pool_info.pools_included.begin(), pool_info.pools_included.end(), sel.value());
        pool_up_button->set_sensitive(it != pool_info.pools_included.begin());
        pool_down_button->set_sensitive(it != (pool_info.pools_included.end() - 1));
    }
    else {
        pool_up_button->set_sensitive(false);
        pool_down_button->set_sensitive(false);
    }
}


} // namespace horizon
