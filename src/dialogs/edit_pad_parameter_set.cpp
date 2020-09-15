#include "edit_pad_parameter_set.hpp"
#include "widgets/parameter_set_editor.hpp"
#include "widgets/pool_browser_button.hpp"
#include "package/pad.hpp"
#include "widgets/pool_browser_padstack.hpp"
#include "pool/package.hpp"
#include "util/util.hpp"
#include "pool/ipool.hpp"
#include <iostream>
#include <deque>
#include <algorithm>

namespace horizon {

class MyParameterSetEditor : public ParameterSetEditor {
public:
private:
    using ParameterSetEditor::ParameterSetEditor;
    Gtk::Widget *create_extra_widget(ParameterID id) override
    {
        auto w = Gtk::manage(new Gtk::Button());
        w->set_image_from_icon_name("object-select-symbolic", Gtk::ICON_SIZE_BUTTON);
        w->set_tooltip_text("Apply to all selected pads (Shift+Enter)");
        w->signal_clicked().connect([this, id] { s_signal_apply_all.emit(id); });
        return w;
    }
};

PadParameterSetDialog::PadParameterSetDialog(Gtk::Window *parent, std::set<class Pad *> &pads, IPool &p,
                                             class Package &pkg)
    : Gtk::Dialog("Edit pad", *parent, Gtk::DialogFlags::DIALOG_MODAL | Gtk::DialogFlags::DIALOG_USE_HEADER_BAR),
      pool(p)
{
    set_default_size(300, 600);
    add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
    add_button("OK", Gtk::ResponseType::RESPONSE_OK);
    set_default_response(Gtk::ResponseType::RESPONSE_OK);

    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0));
    auto combo = Gtk::manage(new Gtk::ComboBoxText());
    combo->set_margin_start(8);
    combo->set_margin_end(8);
    combo->set_margin_top(8);
    combo->set_margin_bottom(8);
    box->pack_start(*combo, false, false, 0);

    auto box2 = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
    box2->set_margin_start(8);
    box2->set_margin_end(8);
    {
        auto la = Gtk::manage(new Gtk::Label("Padstack"));
        la->get_style_context()->add_class("dim-label");
        box2->pack_start(*la, false, false, 0);
    }

    auto padstack_apply_all_button = Gtk::manage(new Gtk::Button);
    padstack_apply_all_button->set_image_from_icon_name("object-select-symbolic", Gtk::ICON_SIZE_BUTTON);
    padstack_apply_all_button->set_tooltip_text("Apply to all pads");
    padstack_apply_all_button->signal_clicked().connect([this, pads] {
        auto ps = pool.get_padstack(padstack_button->property_selected_uuid());
        for (auto &it : pads) {
            it->pool_padstack = ps;
            it->padstack = *ps;
        }
    });
    box2->pack_end(*padstack_apply_all_button, false, false, 0);

    box->pack_start(*box2, false, false, 0);

    std::map<UUID, Pad *> padmap;
    std::deque<Pad *> pads_sorted(pads.begin(), pads.end());
    std::sort(pads_sorted.begin(), pads_sorted.end(),
              [](const auto a, const auto b) { return strcmp_natural(a->name, b->name) < 0; });

    for (auto it : pads_sorted) {
        combo->append(static_cast<std::string>(it->uuid), it->name);
        padmap.emplace(it->uuid, it);
    }

    combo->signal_changed().connect([this, combo, padmap, box, box2, pads, &pkg] {
        UUID uu(combo->get_active_id());
        auto pad = padmap.at(uu);
        if (editor)
            delete editor;
        editor = Gtk::manage(new MyParameterSetEditor(&pad->parameter_set, false));
        editor->populate();
        editor->signal_apply_all().connect([pads, pad](ParameterID id) {
            for (auto it : pads) {
                it->parameter_set[id] = pad->parameter_set.at(id);
            }
        });
        editor->signal_activate_last().connect([this] { response(Gtk::RESPONSE_OK); });
        box->pack_start(*editor, true, true, 0);
        editor->show();

        if (padstack_button)
            delete padstack_button;

        padstack_button = Gtk::manage(new PoolBrowserButton(ObjectType::PADSTACK, pool));
        auto &br = dynamic_cast<PoolBrowserPadstack &>(padstack_button->get_browser());
        br.set_package_uuid(pkg.uuid);
        padstack_button->property_selected_uuid() = pad->pool_padstack->uuid;
        padstack_button->property_selected_uuid().signal_changed().connect([this, pad] {
            auto ps = pool.get_padstack(padstack_button->property_selected_uuid());
            pad->pool_padstack = ps;
            pad->padstack = *ps;
        });
        box2->pack_start(*padstack_button, true, true, 0);
        padstack_button->show();
    });

    combo->set_active(0);
    editor->focus_first();

    get_content_area()->pack_start(*box, true, true, 0);
    get_content_area()->set_border_width(0);


    show_all();
}
} // namespace horizon
