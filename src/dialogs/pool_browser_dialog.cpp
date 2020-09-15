#include "pool_browser_dialog.hpp"
#include "widgets/pool_browser_unit.hpp"
#include "widgets/pool_browser_part.hpp"
#include "widgets/pool_browser_entity.hpp"
#include "widgets/pool_browser_symbol.hpp"
#include "widgets/pool_browser_package.hpp"
#include "widgets/pool_browser_padstack.hpp"
#include "widgets/pool_browser_frame.hpp"
#include "widgets/pool_browser_decal.hpp"
#include "widgets/preview_canvas.hpp"

namespace horizon {
PoolBrowserDialog::PoolBrowserDialog(Gtk::Window *parent, ObjectType type, IPool &ipool, bool use_preview)
    : Gtk::Dialog("Select Something", Gtk::DialogFlags::DIALOG_MODAL | Gtk::DialogFlags::DIALOG_USE_HEADER_BAR),
      pool(ipool)
{
    if (parent) {
        set_transient_for(*parent);
    }
    Gtk::Button *button_ok = add_button("OK", Gtk::ResponseType::RESPONSE_OK);
    add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
    set_default_response(Gtk::ResponseType::RESPONSE_OK);
    set_default_size(300, 300);
    button_ok->set_sensitive(false);

    bool layered = false;
    switch (type) {
    case ObjectType::UNIT:
        browser = Gtk::manage(new PoolBrowserUnit(pool));
        set_title("Select Unit");
        use_preview = false;
        break;
    case ObjectType::PART:
        browser = Gtk::manage(new PoolBrowserPart(pool));
        set_title("Select Part");
        use_preview = false;
        break;
    case ObjectType::ENTITY:
        browser = Gtk::manage(new PoolBrowserEntity(pool));
        set_title("Select Entity");
        use_preview = false;
        break;
    case ObjectType::SYMBOL:
        browser = Gtk::manage(new PoolBrowserSymbol(pool));
        set_title("Select Symbol");
        break;
    case ObjectType::PACKAGE:
        browser = Gtk::manage(new PoolBrowserPackage(pool));
        set_title("Select Package");
        layered = true;
        break;
    case ObjectType::PADSTACK:
        browser = Gtk::manage(new PoolBrowserPadstack(pool));
        set_title("Select Padstack");
        layered = true;
        break;
    case ObjectType::FRAME:
        browser = Gtk::manage(new PoolBrowserFrame(pool));
        set_title("Select Frame");
        break;
    case ObjectType::DECAL:
        browser = Gtk::manage(new PoolBrowserDecal(pool));
        set_title("Select Decal");
        layered = true;
        break;
    default:;
    }
    browser->search_once();

    if (!use_preview) {
        get_content_area()->pack_start(*browser, true, true, 0);
    }
    else {
        set_default_size(1000, 500);
        auto paned = Gtk::manage(new Gtk::Paned(Gtk::ORIENTATION_HORIZONTAL));
        paned->add1(*browser);

        auto preview = Gtk::manage(new PreviewCanvas(pool, layered));
        browser->signal_selected().connect([this, preview, type] { preview->load(type, browser->get_selected()); });
        paned->add2(*preview);
        get_content_area()->pack_start(*paned, true, true, 0);
    }
    get_content_area()->set_border_width(0);


    browser->signal_activated().connect([this] { response(Gtk::ResponseType::RESPONSE_OK); });

    browser->signal_selected().connect([button_ok, this] { button_ok->set_sensitive(browser->get_any_selected()); });

    get_content_area()->show_all();
}


PoolBrowser &PoolBrowserDialog::get_browser()
{
    assert(browser);
    return *browser;
}
} // namespace horizon
