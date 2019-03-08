#include "create_part_dialog.hpp"
#include "widgets/pool_browser_entity.hpp"
#include "widgets/pool_browser_package.hpp"

namespace horizon {
CreatePartDialog::CreatePartDialog(Gtk::Window *parent, Pool *ipool, const UUID &entity_uuid, const UUID &package_uuid)
    : Gtk::Dialog("Select Entity and Package", *parent,
                  Gtk::DialogFlags::DIALOG_MODAL | Gtk::DialogFlags::DIALOG_USE_HEADER_BAR),
      pool(ipool)
{
    button_ok = add_button("OK", Gtk::ResponseType::RESPONSE_OK);
    add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
    set_default_response(Gtk::ResponseType::RESPONSE_OK);
    set_default_size(300, 600);
    button_ok->set_sensitive(false);

    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 0));

    browser_entity = Gtk::manage(new PoolBrowserEntity(pool));
    browser_package = Gtk::manage(new PoolBrowserPackage(pool));
    browser_entity->go_to(entity_uuid);
    browser_package->go_to(package_uuid);

    auto boxe = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 4));
    auto lae = Gtk::manage(new Gtk::Label());
    lae->set_markup("<b>Entity</b>");
    boxe->pack_start(*lae, false, false, 0);
    boxe->pack_start(*browser_entity, true, true, 0);
    boxe->set_margin_top(20);
    box->pack_start(*boxe, true, true, 0);

    auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_VERTICAL));
    box->pack_start(*sep, false, false, 0);

    auto boxp = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 4));
    auto lap = Gtk::manage(new Gtk::Label());
    lap->set_markup("<b>Package</b>");
    boxp->pack_start(*lap, false, false, 0);
    boxp->pack_start(*browser_package, true, true, 0);
    boxp->set_margin_top(20);
    box->pack_start(*boxp, true, true, 0);


    get_content_area()->pack_start(*box, true, true, 0);
    get_content_area()->set_border_width(0);

    browser_entity->signal_activated().connect(sigc::mem_fun(*this, &CreatePartDialog::check_activate));
    browser_package->signal_activated().connect(sigc::mem_fun(*this, &CreatePartDialog::check_activate));

    browser_entity->signal_selected().connect(sigc::mem_fun(*this, &CreatePartDialog::check_select));
    browser_package->signal_selected().connect(sigc::mem_fun(*this, &CreatePartDialog::check_select));
    check_select();
    show_all();
}

void CreatePartDialog::check_select()
{
    button_ok->set_sensitive(browser_entity->get_selected() && browser_package->get_selected());
}

void CreatePartDialog::check_activate()
{
    if (browser_entity->get_selected() && browser_package->get_selected())
        response(Gtk::RESPONSE_OK);
}

UUID CreatePartDialog::get_entity()
{
    return browser_entity->get_selected();
}

UUID CreatePartDialog::get_package()
{
    return browser_package->get_selected();
}
} // namespace horizon
