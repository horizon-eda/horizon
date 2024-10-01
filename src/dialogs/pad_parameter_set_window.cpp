#include "pad_parameter_set_window.hpp"
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

PadParameterSetWindow::PadParameterSetWindow(Gtk::Window *parent, class ImpInterface *intf,
                                             std::set<class Pad *> &apads, IPool &p, class Package &apkg)
    : ToolWindow(parent, intf), pool(p), pkg(apkg), pads(apads)
{
    set_default_size(300, 600);
    set_title("Edit pad");

    box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0));
    combo = Gtk::manage(new Gtk::ComboBoxText());
    combo->set_margin_start(8);
    combo->set_margin_end(8);
    combo->set_margin_top(8);
    combo->set_margin_bottom(8);
    box->pack_start(*combo, false, false, 0);

    box2 = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
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
    padstack_apply_all_button->signal_clicked().connect([this] {
        auto ps = pool.get_padstack(padstack_button->property_selected_uuid());
        for (auto &it : pads) {
            it->pool_padstack = ps;
            it->padstack = *ps;
            auto ps_this = it->parameter_set;
            copy_param(ps_this, pkg.parameter_set, it->parameters_fixed,
                       {ParameterID::SOLDER_MASK_EXPANSION, ParameterID::PASTE_MASK_CONTRACTION,
                        ParameterID::HOLE_SOLDER_MASK_EXPANSION});
            it->padstack.apply_parameter_set(ps_this);
        }
        emit_event(ToolDataWindow::Event::UPDATE);
    });
    box2->pack_end(*padstack_apply_all_button, false, false, 0);

    box->pack_start(*box2, false, false, 0);

    std::vector<Pad *> pads_sorted(pads.begin(), pads.end());
    std::sort(pads_sorted.begin(), pads_sorted.end(),
              [](const auto a, const auto b) { return strcmp_natural(a->name, b->name) < 0; });

    for (auto it : pads_sorted) {
        combo->append(static_cast<std::string>(it->uuid), it->name);
    }

    combo->signal_changed().connect(sigc::mem_fun(*this, &PadParameterSetWindow::load_pad));

    combo->set_active(0);
    editor->focus_first();

    add(*box);

    show_all();
}

void PadParameterSetWindow::load_pad()
{
    UUID uu(combo->get_active_id());
    pad_current = &pkg.pads.at(uu);
    unset_focus();
    if (editor)
        delete editor;
    editor = Gtk::manage(new ParameterSetEditor(&pad_current->parameter_set, false));
    editor->set_has_apply_all_toggle("Apply to all selected pads (Shift+Enter)");
    editor->signal_create_extra_widget().connect([this](ParameterID id) {
        auto w = Gtk::manage(new Gtk::CheckButton("Fixed"));
        w->set_tooltip_text("Fixed pad parameters cannot be changed by package or board rules");
        w->set_active(pad_current->parameters_fixed.count(id));
        w->signal_toggled().connect([this, id, w] {
            if (w->get_active()) {
                pad_current->parameters_fixed.insert(id);
            }
            else {
                pad_current->parameters_fixed.erase(id);
            }
            if (params_apply_all.count(id)) {
                apply_all(id);
                for (auto pad : pads) {
                    auto ps_this = pad->parameter_set;
                    copy_param(ps_this, pkg.parameter_set, pad->parameters_fixed,
                               {ParameterID::SOLDER_MASK_EXPANSION, ParameterID::PASTE_MASK_CONTRACTION,
                                ParameterID::HOLE_SOLDER_MASK_EXPANSION});
                    pad->padstack.apply_parameter_set(ps_this);
                }
            }
            else {
                auto ps_current = pad_current->parameter_set;
                copy_param(ps_current, pkg.parameter_set, pad_current->parameters_fixed,
                           {ParameterID::SOLDER_MASK_EXPANSION, ParameterID::PASTE_MASK_CONTRACTION,
                            ParameterID::HOLE_SOLDER_MASK_EXPANSION});
                pad_current->padstack.apply_parameter_set(ps_current);
            }
            emit_event(ToolDataWindow::Event::UPDATE);
        });
        return w;
    });
    editor->signal_remove_extra_widget().connect([this](ParameterID id) { pad_current->parameters_fixed.erase(id); });
    editor->populate();
    editor->set_apply_all(params_apply_all);
    editor->signal_apply_all_toggled().connect([this](ParameterID id, bool enable) {
        if (enable) {
            params_apply_all.insert(id);
            apply_all(id);
            for (auto pad : pads) {
                auto ps_this = pad->parameter_set;
                copy_param(ps_this, pkg.parameter_set, pad->parameters_fixed,
                           {ParameterID::SOLDER_MASK_EXPANSION, ParameterID::PASTE_MASK_CONTRACTION,
                            ParameterID::HOLE_SOLDER_MASK_EXPANSION});
                pad->padstack.apply_parameter_set(ps_this);
            }
            emit_event(ToolDataWindow::Event::UPDATE);
        }
        else {
            params_apply_all.erase(id);
        }
    });
    editor->signal_activate_last().connect([this] { emit_event(ToolDataWindow::Event::OK); });
    editor->signal_changed().connect([this] {
        for (auto id : params_apply_all) {
            apply_all(id);
        }
        for (auto pad : pads) {
            auto ps_this = pad->parameter_set;
            copy_param(ps_this, pkg.parameter_set, pad->parameters_fixed,
                       {ParameterID::SOLDER_MASK_EXPANSION, ParameterID::PASTE_MASK_CONTRACTION,
                        ParameterID::HOLE_SOLDER_MASK_EXPANSION});
            pad->padstack.apply_parameter_set(ps_this);
        }
        emit_event(ToolDataWindow::Event::UPDATE);
    });
    box->pack_start(*editor, true, true, 0);
    editor->show();

    if (padstack_button)
        delete padstack_button;

    padstack_button = Gtk::manage(new PoolBrowserButton(ObjectType::PADSTACK, pool));
    auto &br = dynamic_cast<PoolBrowserPadstack &>(padstack_button->get_browser());
    br.set_package_uuid(pkg.uuid);
    padstack_button->property_selected_uuid() = pad_current->pool_padstack->uuid;
    padstack_button->property_selected_uuid().signal_changed().connect([this] {
        auto ps = pool.get_padstack(padstack_button->property_selected_uuid());
        pad_current->pool_padstack = ps;
        pad_current->padstack = *ps;
        auto ps_current = pad_current->parameter_set;
        copy_param(ps_current, pkg.parameter_set, pad_current->parameters_fixed,
                   {ParameterID::SOLDER_MASK_EXPANSION, ParameterID::PASTE_MASK_CONTRACTION,
                    ParameterID::HOLE_SOLDER_MASK_EXPANSION});
        pad_current->padstack.apply_parameter_set(ps_current);
        emit_event(ToolDataWindow::Event::UPDATE);
    });
    box2->pack_start(*padstack_button, true, true, 0);
    padstack_button->show();
}

void PadParameterSetWindow::apply_all(ParameterID id)
{
    for (auto it : pads) {
        it->parameter_set[id] = pad_current->parameter_set.at(id);
        if (pad_current->parameters_fixed.count(id)) {
            it->parameters_fixed.insert(id);
        }
        else {
            it->parameters_fixed.erase(id);
        }
    }
}

bool PadParameterSetWindow::go_to_pad(const UUID &uu)
{
    const auto pad = &pkg.pads.at(uu);
    if (!pads.count(pad))
        return false;

    combo->set_active_id((std::string)pad->uuid);
    return true;
}

} // namespace horizon
