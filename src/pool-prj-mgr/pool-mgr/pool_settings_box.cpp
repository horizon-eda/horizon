#include "pool_settings_box.hpp"
#include "pool_notebook.hpp"
#include "pool/pool_manager.hpp"

namespace horizon {

PoolSettingsBox::PoolSettingsBox(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, PoolNotebook *nb,
                                 const std::string &bp)
    : Gtk::Box(cobject), notebook(nb), pool_base_path(bp)
{
    x->get_widget("button_save_pool", save_button);
    x->get_widget("pool_settings_name_entry", entry_name);
    x->get_widget("pool_settings_available_listbox", pools_available_listbox);
    x->get_widget("pool_settings_included_listbox", pools_included_listbox);
    x->get_widget("pool_inc_button", pool_inc_button);
    x->get_widget("pool_excl_button", pool_excl_button);
    x->get_widget("pool_up_button", pool_up_button);
    x->get_widget("pool_down_button", pool_down_button);
    x->get_widget("pool_settings_hint_label", hint_label);
    hint_label->hide();
    entry_name->set_text(PoolManager::get().get_pools().at(bp).name);
    entry_name->signal_changed().connect(sigc::mem_fun(*this, &PoolSettingsBox::set_needs_save));
    save_button->signal_clicked().connect(sigc::mem_fun(*this, &PoolSettingsBox::save));
    save_button->set_sensitive(false);

    PoolManager::get().signal_changed().connect(sigc::mem_fun(*this, &PoolSettingsBox::update_pools));

    pool_inc_button->signal_clicked().connect([this] { inc_excl_pool(true); });
    pool_excl_button->signal_clicked().connect([this] { inc_excl_pool(false); });

    pools_included = PoolManager::get().get_pools().at(bp).pools_included;

    update_pools();
}

PoolSettingsBox *PoolSettingsBox::create(PoolNotebook *nb, const std::string &bp)
{
    PoolSettingsBox *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    std::vector<Glib::ustring> widgets = {"box_settings", "image1", "image2", "image3", "image4"};
    x->add_from_resource("/org/horizon-eda/horizon/pool-prj-mgr/window.ui", widgets);
    x->get_widget_derived("box_settings", w, nb, bp);
    w->reference();
    return w;
}

void PoolSettingsBox::save()
{
    PoolManagerPool p(PoolManager::get().get_pools().at(pool_base_path));
    p.name = entry_name->get_text();
    p.pools_included = pools_included;
    PoolManager::get().update_pool(pool_base_path, p);
    save_button->set_sensitive(false);
    hint_label->set_markup("Almost there! For the items of the included pool to show up, click <i>Update pool</i>.");
    needs_save = false;
}

void PoolSettingsBox::set_needs_save()
{
    needs_save = true;
    save_button->set_sensitive(true);
    hint_label->set_markup("For the items of the included pool to show up, click <i>Save</i> and <i>Update pool</i>.");
    hint_label->show();
}

bool PoolSettingsBox::get_needs_save() const
{
    return needs_save;
}

void PoolSettingsBox::pool_updated()
{
    if (!needs_save)
        hint_label->hide();
}

class PoolListItem : public Gtk::Box {
public:
    PoolListItem(const UUID &uu);
    const UUID uuid;
};

PoolListItem::PoolListItem(const UUID &uu) : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 5), uuid(uu)
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
        la->set_xalign(0);
        pack_start(*la, true, true, 0);
        la->show();
    }
}

void PoolSettingsBox::inc_excl_pool(bool inc)
{
    Gtk::ListBox *lb;
    if (inc)
        lb = pools_available_listbox;
    else
        lb = pools_included_listbox;

    if (auto row = lb->get_selected_row()) {
        auto it = dynamic_cast<PoolListItem *>(row->get_child());
        UUID uu = it->uuid;
        if (inc) {
            pools_included.push_back(uu);
        }
        else {
            pools_included.erase(std::remove(pools_included.begin(), pools_included.end(), uu), pools_included.end());
        }
        update_pools();
    }
    set_needs_save();
}

void PoolSettingsBox::update_pools()
{
    for (auto &it : pools_available_listbox->get_children()) {
        delete it;
    }
    for (const auto &it : PoolManager::get().get_pools()) {
        if (it.second.enabled && it.second.uuid != PoolManager::get().get_pools().at(pool_base_path).uuid
            && std::count(pools_included.begin(), pools_included.end(), it.second.uuid) == 0) {
            auto w = Gtk::manage(new PoolListItem(it.second.uuid));
            pools_available_listbox->append(*w);
            w->show();
        }
    }
    for (auto &it : pools_included_listbox->get_children()) {
        delete it;
    }
    for (const auto &uu : pools_included) {
        auto w = Gtk::manage(new PoolListItem(uu));
        pools_included_listbox->append(*w);
        w->show();
    }
}

} // namespace horizon
