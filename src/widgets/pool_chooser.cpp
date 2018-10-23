#include "pool_chooser.hpp"
#include "pool/pool_manager.hpp"

namespace horizon {

class PoolChooserDialogRow : public Gtk::Box {
public:
    PoolChooserDialogRow(const PoolManagerPool &pool);
    const UUID uuid;
};

PoolChooserDialogRow::PoolChooserDialogRow(const PoolManagerPool &pool)
    : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 5), uuid(pool.uuid)
{
    property_margin() = 5;
    {
        auto la = Gtk::manage(new Gtk::Label);
        la->set_xalign(0);
        la->set_markup("<b>" + pool.name + "</b>");
        pack_start(*la, false, false, 0);
        la->show();
    }
    {
        auto la = Gtk::manage(new Gtk::Label);
        la->set_xalign(0);
        la->set_text((std::string)pool.uuid);
        pack_start(*la, false, false, 0);
        la->show();
    }
    {
        auto la = Gtk::manage(new Gtk::Label);
        la->set_xalign(0);
        la->set_text(pool.base_path);
        pack_start(*la, false, false, 0);
        la->show();
    }
}

PoolChooserDialog::PoolChooserDialog(Gtk::Window *parent, const std::string &msg)
    : Gtk::Dialog("Choose pool", *parent, Gtk::DIALOG_MODAL | Gtk::DIALOG_USE_HEADER_BAR)
{
    add_button("Cancel", Gtk::RESPONSE_CANCEL);
    auto ok_button = add_button("OK", Gtk::RESPONSE_OK);
    set_default_response(Gtk::RESPONSE_OK);
    auto sc = Gtk::manage(new Gtk::ScrolledWindow);
    sc->set_propagate_natural_height(true);
    sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    lb = Gtk::manage(new Gtk::ListBox);
    lb->set_activate_on_single_click(false);
    lb->set_selection_mode(Gtk::SELECTION_BROWSE);
    lb->signal_row_activated().connect([this](Gtk::ListBoxRow *row) { response(Gtk::RESPONSE_OK); });
    sc->add(*lb);

    {
        auto la = Gtk::manage(new Gtk::Label("You haven't set up any pools, add some in the preferences dialog"));
        la->get_style_context()->add_class("dim-label");
        la->set_hexpand(false);
        la->set_hexpand_set(true);
        la->set_max_width_chars(10);
        la->property_margin() = 5;
        la->set_line_wrap(true);
        lb->set_placeholder(*la);
        la->show();
    }

    const auto &pools = PoolManager::get().get_pools();
    ok_button->set_sensitive(
            std::count_if(pools.begin(), pools.end(), [](const auto &it) { return it.second.enabled; }));
    for (const auto &it : pools) {
        if (it.second.enabled) {
            auto w = Gtk::manage(new PoolChooserDialogRow(it.second));
            lb->append(*w);
            w->show();
        }
    }
    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0));
    if (msg.size()) {
        auto la = Gtk::manage(new Gtk::Label(msg));
        la->set_hexpand(false);
        la->set_hexpand_set(true);
        la->set_xalign(0);
        la->set_max_width_chars(10);
        la->property_margin() = 5;
        la->set_line_wrap(true);
        box->pack_start(*la, false, false, 0);
        auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
        box->pack_start(*sep, false, false, 0);
    }
    box->pack_start(*sc, true, true, 0);
    box->show_all();
    get_content_area()->pack_start(*box, true, true, 0);
    get_content_area()->set_border_width(0);
}

void PoolChooserDialog::select_pool(const UUID &uu)
{
    for (auto ch : lb->get_children()) {
        auto row = dynamic_cast<Gtk::ListBoxRow *>(ch);
        auto w = dynamic_cast<PoolChooserDialogRow *>(row->get_child());
        if (w->uuid == uu) {
            lb->select_row(*row);
            break;
        }
    }
}

UUID PoolChooserDialog::get_selected_pool()
{
    auto row = lb->get_selected_row();
    if (row) {
        return dynamic_cast<PoolChooserDialogRow *>(row->get_child())->uuid;
    }
    else {
        return UUID();
    }
}
} // namespace horizon
