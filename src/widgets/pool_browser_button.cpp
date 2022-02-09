#include "pool_browser_button.hpp"
#include "pool/padstack.hpp"
#include "pool/package.hpp"
#include "pool/ipool.hpp"
#include "pool/part.hpp"
#include "pool_browser.hpp"
#include "frame/frame.hpp"

namespace horizon {
PoolBrowserButton::PoolBrowserButton(ObjectType ty, IPool &ipool)
    : Glib::ObjectBase(typeid(PoolBrowserButton)), Gtk::Button(), p_property_selected_uuid(*this, "selected-uuid"),
      pool(ipool), type(ty), dia(nullptr, type, pool)
{
    label = Gtk::manage(new Gtk::Label);
    label->set_ellipsize(Pango::ELLIPSIZE_END);
    label->show();
    label->set_xalign(0);
    add(*label);

    update_label();
    property_selected_uuid().signal_changed().connect([this] {
        selected_uuid = property_selected_uuid();
        update_label();
    });
}

class PoolBrowser &PoolBrowserButton::get_browser()
{
    return dia.get_browser();
}

void PoolBrowserButton::on_clicked()
{
    Gtk::Button::on_clicked();
    auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));
    dia.set_transient_for(*top);
    dia.get_browser().go_to(p_property_selected_uuid.get_value());
    if (dia.run() == Gtk::RESPONSE_OK) {
        selected_uuid = dia.get_browser().get_selected();
        p_property_selected_uuid.set_value(selected_uuid);
        update_label();
    }
    dia.hide();
}

void PoolBrowserButton::update_label()
{
    if (!selected_uuid) {
        label->set_text("(None)");
        return;
    }
    switch (type) {
    case ObjectType::PADSTACK:
        label->set_text(pool.get_padstack(selected_uuid)->name);
        break;
    case ObjectType::PACKAGE:
        label->set_text(pool.get_package(selected_uuid)->name);
        break;
    case ObjectType::FRAME:
        label->set_text(pool.get_frame(selected_uuid)->name);
        break;
    case ObjectType::PART:
        label->set_text(pool.get_part(selected_uuid)->get_MPN());
        break;
    default:
        label->set_text("fixme");
    }
}
} // namespace horizon
