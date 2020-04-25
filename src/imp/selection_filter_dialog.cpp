#include "selection_filter_dialog.hpp"
#include "util/gtk_util.hpp"
#include "common/common.hpp"
#include "canvas/selection_filter.hpp"
#include "imp/imp.hpp"
#include "board/board_layers.hpp"

namespace horizon {


static const auto sub_margin = 35;


SelectionFilterDialog::SelectionFilterDialog(Gtk::Window *parent, SelectionFilter &sf, ImpBase &aimp)
    : Gtk::Window(), selection_filter(sf), imp(aimp)
{
    set_default_size(220, 300);
    set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
    set_transient_for(*parent);
    auto hb = Gtk::manage(new Gtk::HeaderBar());
    hb->set_show_close_button(true);
    set_titlebar(*hb);
    hb->show();
    set_title("Selection filter");
    install_esc_to_close(*this);

    reset_button = Gtk::manage(new Gtk::Button());
    reset_button->set_image_from_icon_name("object-select-symbolic", Gtk::ICON_SIZE_BUTTON);
    reset_button->set_tooltip_text("Select all");
    reset_button->show_all();
    reset_button->signal_clicked().connect([this] {
        set_all(true);
        update();
    });
    hb->pack_start(*reset_button);
    reset_button->show();

    listbox = Gtk::manage(new Gtk::ListBox());
    listbox->set_selection_mode(Gtk::SELECTION_NONE);
    listbox->set_header_func(sigc::ptr_fun(&header_func_separator));
    auto sg = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);
    for (const auto &it : imp.get_selection_filter_info()) {
        auto ot = it.first;

        {
            auto cb = Gtk::manage(new Gtk::CheckButton(object_descriptions.at(it.first).name_pl));
            cb->set_active(true);
            if (it.second.layers.size() == 0) {
                cb->signal_toggled().connect([this, ot, cb] {
                    update_filter();
                    update();
                });
            }
            else {
                cb->signal_toggled().connect([this, ot, cb] {
                    if (checkbuttons[ot].blocked)
                        return;
                    checkbuttons[ot].blocked = true;
                    for (auto &cbl : checkbuttons[ot].layer_buttons) {
                        cbl.second->set_active(cb->get_active());
                    }
                    if (checkbuttons[ot].other_layer_checkbutton) {
                        checkbuttons[ot].other_layer_checkbutton->set_active(cb->get_active());
                    }
                    checkbuttons[ot].blocked = false;
                    update();
                });
            }
            connect_doubleclick(cb);

            checkbuttons[ot].checkbutton = cb;
            auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 3));
            box->property_margin() = 3;
            if (it.second.layers.size() != 0) {
                auto expand_button = Gtk::manage(new Gtk::ToggleButton);
                checkbuttons.at(ot).expand_button = expand_button;
                expand_button->set_image_from_icon_name("pan-end-symbolic", Gtk::ICON_SIZE_BUTTON);
                expand_button->get_style_context()->add_class("imp-selection-filter-tiny-button");
                expand_button->set_relief(Gtk::RELIEF_NONE);
                sg->add_widget(*expand_button);
                box->pack_start(*expand_button, false, false, 0);
                expand_button->signal_toggled().connect([this, expand_button, ot] {
                    auto show = expand_button->get_active();
                    checkbuttons.at(ot).expanded = show;
                    if (show) {
                        expand_button->set_image_from_icon_name("pan-down-symbolic", Gtk::ICON_SIZE_BUTTON);
                    }
                    else {
                        expand_button->set_image_from_icon_name("pan-end-symbolic", Gtk::ICON_SIZE_BUTTON);
                    }
                    for (auto &it_cb : checkbuttons.at(ot).layer_buttons) {
                        it_cb.second->get_parent()->set_visible(show);
                    }
                    if (checkbuttons.at(ot).other_layer_checkbutton) {
                        checkbuttons.at(ot).other_layer_checkbutton->get_parent()->set_visible(show);
                    }
                });
            }
            else {
                auto placeholder = Gtk::manage(new Gtk::Label);
                sg->add_widget(*placeholder);
                box->pack_start(*placeholder, false, false, 0);
            }
            box->pack_start(*cb, true, true, 0);
            box->show_all();
            listbox->append(*box);
        }
        for (auto layer : it.second.layers) {
            add_layer_button(ot, layer, -1);
        }
        if (it.second.has_others) {
            auto cbl = Gtk::manage(new Gtk::CheckButton("Others"));
            cbl->property_margin() = 3;
            cbl->set_margin_start(sub_margin);
            cbl->set_active(true);
            checkbuttons[ot].other_layer_checkbutton = cbl;
            listbox->append(*cbl);
            cbl->show();
            cbl->get_parent()->set_visible(false);
            connect_doubleclick(cbl);
            cbl->signal_toggled().connect([this, ot, cbl] {
                update_filter();
                if (!checkbuttons[ot].blocked)
                    update();
            });
        }
    }
    listbox->set_no_show_all(true);
    listbox->show();
    auto sc = Gtk::manage(new Gtk::ScrolledWindow());
    sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    sc->add(*listbox);

    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));

    if (imp.is_layered()) {
        work_layer_only_cb = Gtk::manage(new Gtk::CheckButton("Work layer only"));
        work_layer_only_cb->property_margin() = 3;
        work_layer_only_cb->signal_toggled().connect(
                sigc::mem_fun(*this, &SelectionFilterDialog::update_work_layer_only));
        box->pack_start(*work_layer_only_cb, false, false, 0);
        {
            auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
            box->pack_start(*sep, false, false, 0);
        }
    }

    box->pack_start(*sc, true, true, 0);

    {
        auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
        box->pack_start(*sep, false, false, 0);
    }

    {
        auto la = Gtk::manage(new Gtk::Label("Double click to select just one."));
        make_label_small(la);
        la->property_margin() = 3;
        la->get_style_context()->add_class("dim-label");
        box->pack_start(*la, false, false, 0);
    }
    add(*box);
    box->show_all();
    update();
    update_filter();
}

void SelectionFilterDialog::update_filter()
{
    for (const auto &it : checkbuttons) {
        if (it.second.layer_buttons.size() == 0) {
            if (work_layer_only)
                selection_filter.object_filter[it.first].other_layers = false;
            else
                selection_filter.object_filter[it.first].other_layers = it.second.checkbutton->get_active();
        }
        else {
            if (work_layer_only) {
                selection_filter.object_filter[it.first].other_layers = false;
                if (it.second.checkbutton->get_active())
                    selection_filter.object_filter[it.first].layers = {{work_layer, true}};
                else
                    selection_filter.object_filter[it.first].layers.clear();
            }
            else {
                if (it.second.other_layer_checkbutton)
                    selection_filter.object_filter[it.first].other_layers =
                            it.second.other_layer_checkbutton->get_active();
                for (auto &it_la : it.second.layer_buttons) {
                    selection_filter.object_filter[it.first].layers[it_la.first] = it_la.second->get_active();
                }
            }
        }
    }
}

void SelectionFilterDialog::set_work_layer(int layer)
{
    work_layer = layer;
    update_work_layer_only();
}

void SelectionFilterDialog::update_work_layer_only()
{
    if (!work_layer_only_cb)
        return;
    work_layer_only = work_layer_only_cb->get_active();
    if (work_layer_only && !work_layer_only_before) { // got activated, save
        saved.clear();
        for (auto &it : checkbuttons) {
            saved[it.first];
            if (it.second.layer_buttons.size() == 0) {
                if (it.second.checkbutton->get_active())
                    saved.at(it.first).insert(0);
                it.second.checkbutton->set_active(false);
                it.second.checkbutton->set_sensitive(false);
            }
            else {
                if (it.second.expand_button) {
                    if (it.second.expand_button->get_active())
                        saved.at(it.first).insert(10001);
                    it.second.expand_button->set_active(false);
                    it.second.expand_button->set_sensitive(false);
                }
            }
        }
    }

    if (!work_layer_only && work_layer_only_before) { // got deactivated, restore
        for (auto &it : checkbuttons) {
            if (saved.count(it.first)) {
                const auto &sav = saved.at(it.first);
                it.second.checkbutton->set_sensitive(true);
                if (it.second.layer_buttons.size() == 0) {
                    it.second.checkbutton->set_active(sav.count(0));
                }
                else {
                    it.second.expand_button->set_active(sav.count(10001));
                    it.second.expand_button->set_sensitive(true);
                }
            }
        }
    }
    work_layer_only_before = work_layer_only;
    update_filter();
}

bool SelectionFilterDialog::Type::get_all_active()
{
    if (layer_buttons.size() == 0) {
        return checkbutton->get_active();
    }
    bool all_on =
            std::all_of(layer_buttons.begin(), layer_buttons.end(), [](auto &x) { return x.second->get_active(); });
    if (other_layer_checkbutton) {
        all_on = all_on && other_layer_checkbutton->get_active();
    }
    return all_on;
}

void SelectionFilterDialog::Type::update()
{
    if (layer_buttons.size() == 0)
        return;
    blocked = true;
    bool all_on =
            std::all_of(layer_buttons.begin(), layer_buttons.end(), [](auto &x) { return x.second->get_active(); });
    bool all_off =
            std::all_of(layer_buttons.begin(), layer_buttons.end(), [](auto &x) { return !x.second->get_active(); });
    if (other_layer_checkbutton) {
        all_on = all_on && other_layer_checkbutton->get_active();
        all_off = all_off && !other_layer_checkbutton->get_active();
    }
    if (all_on) {
        checkbutton->set_inconsistent(false);
        checkbutton->set_active(true);
    }
    else if (all_off) {
        checkbutton->set_inconsistent(false);
        checkbutton->set_active(false);
    }
    else {
        checkbutton->set_active(true);
        checkbutton->set_inconsistent(true);
    }
    blocked = false;
}


void SelectionFilterDialog::update()
{
    if (!work_layer_only) {
        reset_button->set_sensitive(!std::all_of(checkbuttons.begin(), checkbuttons.end(),
                                                 [](auto x) { return x.second.get_all_active(); }));
    }
    else {
        reset_button->set_sensitive(false);
        for (const auto &it : checkbuttons) {
            if (it.second.layer_buttons.size()) {
                if (!it.second.checkbutton->get_active()) {
                    reset_button->set_sensitive(true);
                    break;
                }
            }
        }
    }
    for (auto &it : checkbuttons) {
        if (work_layer_only) {
            it.second.checkbutton->set_inconsistent(false);
        }
        else {
            it.second.update();
        }
    }
    s_signal_changed.emit();
}

void SelectionFilterDialog::set_all(bool state)
{
    for (auto &it : checkbuttons) {
        for (auto cb : it.second.layer_buttons) {
            cb.second->set_active(state);
        }
        if (it.second.other_layer_checkbutton)
            it.second.other_layer_checkbutton->set_active(state);
        if (it.second.checkbutton->get_sensitive())
            it.second.checkbutton->set_active(state);
    }
}

void SelectionFilterDialog::connect_doubleclick(Gtk::CheckButton *cb)
{
    cb->signal_button_press_event().connect(
            [this, cb](GdkEventButton *ev) {
                if (ev->type == GDK_2BUTTON_PRESS) {
                    set_all(false);
                    cb->set_active(true);
                }
                else if (ev->type == GDK_BUTTON_PRESS) {
                    cb->set_active(!cb->get_active());
                }
                update();
                return true;
            },
            false);
}

Gtk::CheckButton *SelectionFilterDialog::add_layer_button(ObjectType type, int layer, int index, bool active)
{
    auto cbl = Gtk::manage(new Gtk::CheckButton(BoardLayers::get_layer_name(layer)));
    cbl->property_margin() = 3;
    cbl->set_margin_start(sub_margin);
    checkbuttons[type].layer_buttons[layer] = cbl;
    listbox->insert(*cbl, index);
    cbl->show();
    cbl->get_parent()->set_visible(false);
    cbl->set_active(active);
    connect_doubleclick(cbl);
    cbl->signal_toggled().connect([this, type, layer, cbl] {
        update_filter();
        if (!checkbuttons[type].blocked)
            update();
    });
    update_filter();
    return cbl;
}

void SelectionFilterDialog::update_layers()
{
    auto info = imp.get_selection_filter_info();
    for (auto &it_type : checkbuttons) {
        std::set<int> layers_current;
        std::set<int> layers_new;
        std::transform(it_type.second.layer_buttons.begin(), it_type.second.layer_buttons.end(),
                       std::inserter(layers_current, layers_current.begin()), [](auto &x) { return x.first; });
        std::transform(info.at(it_type.first).layers.begin(), info.at(it_type.first).layers.end(),
                       std::inserter(layers_new, layers_new.begin()), [](auto &x) { return x; });
        if (layers_new != layers_current) {
            std::map<int, bool> state;
            for (auto &it : it_type.second.layer_buttons) {
                state[it.first] = it.second->get_active();
                delete it.second->get_parent();
            }
            it_type.second.layer_buttons.clear();
            auto inf = info.at(it_type.first);
            int index = dynamic_cast<Gtk::ListBoxRow *>(it_type.second.checkbutton->get_ancestor(GTK_TYPE_LIST_BOX_ROW))
                                ->get_index();
            for (auto layer : inf.layers) {
                index++;
                bool active = true;
                if (state.count(layer))
                    active = state.at(layer);
                auto cb = add_layer_button(it_type.first, layer, index, active);
                cb->get_parent()->set_visible(it_type.second.expanded);
            }
        }
    }
}

bool SelectionFilterDialog::get_filtered()
{
    return work_layer_only
           || !std::all_of(checkbuttons.begin(), checkbuttons.end(), [](auto x) { return x.second.get_all_active(); });
}

} // namespace horizon
