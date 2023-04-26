#include "parameter_set_editor.hpp"
#include "spin_button_dim.hpp"
#include "common/common.hpp"

namespace horizon {

static void header_fun(Gtk::ListBoxRow *row, Gtk::ListBoxRow *before)
{
    if (before && !row->get_header()) {
        auto ret = Gtk::manage(new Gtk::Separator);
        row->set_header(*ret);
    }
}

class ParameterEditor : public Gtk::Box {
public:
    ParameterEditor(ParameterID id, ParameterSetEditor *pse)
        : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8), parameter_id(id), parent(pse)
    {
        auto la = Gtk::manage(new Gtk::Label(parameter_id_to_name(id)));
        la->get_style_context()->add_class("dim-label");
        la->set_xalign(1);
        parent->sg_label->add_widget(*la);
        pack_start(*la, false, false, 0);

        sp = Gtk::manage(new SpinButtonDim());
        sp->set_range(-1e9, 1e9);
        sp->set_value(parent->parameter_set->at(id));
        sp->signal_value_changed().connect([this] {
            (*parent->parameter_set)[parameter_id] = sp->get_value_as_int();
            parent->s_signal_changed.emit();
        });
        sp->signal_key_press_event().connect([this](GdkEventKey *ev) {
            if (ev->keyval == GDK_KEY_Return && (ev->state & GDK_SHIFT_MASK)) {
                sp->activate();
                parent->s_signal_apply_all.emit(parameter_id);
                parent->s_signal_changed.emit();
                return true;
            }
            return false;
        });
        sp->signal_activate().connect([this] {
            auto widgets = parent->listbox->get_children();
            auto my_row = sp->get_ancestor(GTK_TYPE_LIST_BOX_ROW);
            auto next = std::find(widgets.begin(), widgets.end(), my_row) + 1;
            if (next < widgets.end()) {
                auto e = dynamic_cast<ParameterEditor *>(dynamic_cast<Gtk::ListBoxRow *>(*next)->get_child());
                e->sp->grab_focus();
            }
            else {
                parent->s_signal_activate_last.emit();
            }
        });
        pack_start(*sp, true, true, 0);

        auto delete_button = Gtk::manage(new Gtk::Button());
        delete_button->set_image_from_icon_name("list-remove-symbolic", Gtk::ICON_SIZE_BUTTON);
        pack_start(*delete_button, false, false, 0);
        delete_button->signal_clicked().connect([this] {
            parent->s_signal_remove_extra_widget.emit(parameter_id);
            parent->s_signal_apply_all_toggled.emit(parameter_id, false);
            parent->parameter_set->erase(parameter_id);
            parent->s_signal_changed.emit();
            delete this->get_parent();
        });

        for (auto extra_widget : parent->s_signal_create_extra_widget.emit(id)) {
            if (auto ch = dynamic_cast<Changeable *>(extra_widget)) {
                ch->signal_changed().connect([this] { parent->s_signal_changed.emit(); });
            }
            pack_start(*extra_widget, false, false, 0);
        }
        if (auto bu = parent->create_apply_all_button(id)) {
            apply_all_toggle_button = dynamic_cast<Gtk::ToggleButton *>(bu);
            pack_start(*bu, false, false, 0);
        }

        set_margin_start(4);
        set_margin_end(4);
        set_margin_top(4);
        set_margin_bottom(4);
        show_all();
    }
    SpinButtonDim *sp = nullptr;
    Gtk::ToggleButton *apply_all_toggle_button = nullptr;


    const ParameterID parameter_id;

private:
    ParameterSetEditor *parent;
};

ParameterSetEditor::ParameterSetEditor(ParameterSet *ps, bool populate_init, Style style)
    : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0), parameter_set(ps)
{
    add_button = Gtk::manage(new Gtk::MenuButton());
    add_button->set_label("Add parameter");
    add_button->set_halign(Gtk::ALIGN_START);
    add_button->set_margin_bottom(10);
    add_button->set_margin_top(10);
    add_button->set_margin_start(10);
    add_button->set_margin_end(10);
    add_button->show();
    pack_start(*add_button, false, false, 0);


    add_button->set_menu(menu);
    menu.signal_show().connect(sigc::mem_fun(*this, &ParameterSetEditor::update_menu));
    for (int i = 1; i < (static_cast<int>(ParameterID::N_PARAMETERS)); i++) {
        auto id = static_cast<ParameterID>(i);
        auto item = Gtk::manage(new Gtk::MenuItem(parameter_id_to_name(id)));
        item->signal_activate().connect([this, id] {
            (*parameter_set)[id] = 0.5_mm;
            auto pe = Gtk::manage(new ParameterEditor(id, this));
            listbox->add(*pe);
            s_signal_changed.emit();
        });
        menu.append(*item);
        menu_items.emplace(id, *item);
    }

    auto sc = Gtk::manage(new Gtk::ScrolledWindow());
    sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    if (style == Style::BORDER)
        sc->set_shadow_type(Gtk::SHADOW_IN);

    listbox = Gtk::manage(new Gtk::ListBox());
    listbox->set_selection_mode(Gtk::SELECTION_NONE);
    listbox->set_header_func(&header_fun);
    sc->add(*listbox);

    sg_label = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);
    if (populate_init)
        populate();

    sc->show_all();
    if (style == Style::SEPARATOR) {
        auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
        sep->show();
        pack_start(*sep, false, false, 0);
    }
    pack_start(*sc, true, true, 0);
}

void ParameterSetEditor::set_has_apply_all(const std::string &tooltip_text)
{
    apply_all_tooltip_text.emplace(tooltip_text);
}

void ParameterSetEditor::set_has_apply_all_toggle(const std::string &tooltip_text)
{
    apply_all_tooltip_text.emplace(tooltip_text);
    apply_all_toggle = true;
}


void ParameterSetEditor::set_button_margin_left(int margin)
{
    add_button->set_margin_start(margin);
}

void ParameterSetEditor::populate()
{
    for (auto &it : *parameter_set) {
        auto pe = Gtk::manage(new ParameterEditor(it.first, this));
        listbox->add(*pe);
    }
    listbox->show_all();
}

void ParameterSetEditor::focus_first()
{
    auto widgets = listbox->get_children();
    if (widgets.size()) {
        auto w = dynamic_cast<ParameterEditor *>(dynamic_cast<Gtk::ListBoxRow *>(widgets.front())->get_child());
        w->sp->grab_focus();
    }
}

void ParameterSetEditor::add_or_set_parameter(ParameterID param, int64_t value)
{
    if (parameter_set->count(param)) {
        auto rows = listbox->get_children();
        for (auto row : rows) {
            auto w = dynamic_cast<ParameterEditor *>(dynamic_cast<Gtk::ListBoxRow *>(row)->get_child());
            if (w->parameter_id == param)
                w->sp->set_value(value);
        }
    }
    else {
        (*parameter_set)[param] = value;
        auto pe = Gtk::manage(new ParameterEditor(param, this));
        listbox->add(*pe);
    }
    s_signal_changed.emit();
}

void ParameterSetEditor::update_menu()
{
    for (int i = 1; i < (static_cast<int>(ParameterID::N_PARAMETERS)); i++) {
        auto id = static_cast<ParameterID>(i);
        menu_items.at(id).set_visible(parameter_set->count(id) == 0);
    }
}

Gtk::Widget *ParameterSetEditor::create_apply_all_button(ParameterID id)
{
    if (!apply_all_tooltip_text)
        return nullptr;
    if (apply_all_toggle) {
        auto w = Gtk::manage(new Gtk::ToggleButton("All"));
        w->set_tooltip_text(apply_all_tooltip_text.value());
        w->signal_clicked().connect([this, id, w] { s_signal_apply_all_toggled.emit(id, w->get_active()); });
        return w;
    }
    else {
        auto w = Gtk::manage(new Gtk::Button());
        w->set_image_from_icon_name("object-select-symbolic", Gtk::ICON_SIZE_BUTTON);
        w->set_tooltip_text(apply_all_tooltip_text.value());
        w->signal_clicked().connect([this, id] { s_signal_apply_all.emit(id); });
        return w;
    }
}

void ParameterSetEditor::set_apply_all(std::set<ParameterID> params)
{
    auto rows = listbox->get_children();
    for (auto row : rows) {
        auto w = dynamic_cast<ParameterEditor *>(dynamic_cast<Gtk::ListBoxRow *>(row)->get_child());
        if (w->apply_all_toggle_button)
            w->apply_all_toggle_button->set_active(params.count(w->parameter_id));
    }
}

} // namespace horizon
