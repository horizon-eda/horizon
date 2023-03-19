#include "property_editor.hpp"
#include "common/object_descr.hpp"
#include "property_panel.hpp"
#include "property_panels.hpp"
#include "block/block.hpp"
#include "widgets/spin_button_dim.hpp"
#include "widgets/spin_button_angle.hpp"
#include "util/util.hpp"
#include "widgets/text_editor.hpp"
#include "widgets/layer_combo_box.hpp"
#include <algorithm>
#include <iostream>
#include <iomanip>
#include "util/scoped_block.hpp"

namespace horizon {

PropertyEditor::PropertyEditor(ObjectType t, ObjectProperty::ID prop, class PropertyPanel *p)
    : Gtk::Box(Gtk::Orientation::ORIENTATION_VERTICAL, 4), parent(p), property_id(prop), type(t),
      property(object_descriptions.at(type).properties.at(property_id))
{
    auto *hbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4));
    auto *header = Gtk::manage(new Gtk::Label());
    header->set_markup("<b>" + property.label + "</b>");
    header->set_halign(Gtk::ALIGN_START);
    hbox->pack_start(*header, true, true, 0);

    apply_all_button = Gtk::manage(new Gtk::ToggleButton());
    apply_all_button->set_label("All");
    apply_all_button->get_style_context()->add_class("imp-property-all-button");
    apply_all_button->signal_toggled().connect([this] { s_signal_apply_all.emit(); });
    hbox->pack_start(*apply_all_button, false, false, 0);

    pack_start(*hbox, false, false, 0);
}

bool PropertyEditor::get_apply_all()
{
    return apply_all_button->get_active();
}

void PropertyEditor::construct()
{
    auto *ed = create_editor();
    ed->set_halign(Gtk::ALIGN_END);
    pack_start(*ed, false, false, 0);
}

Gtk::Widget *PropertyEditor::create_editor()
{
    Gtk::Label *la = Gtk::manage(new Gtk::Label("fixme"));
    return la;
}

void PropertyEditor::set_can_apply_all(bool v)
{
    if (v) {
        apply_all_button->set_tooltip_text("Apply changes to all selected " + object_descriptions.at(type).name_pl);
        apply_all_button->set_sensitive(!readonly);
    }
    else {
        apply_all_button->set_tooltip_text("Disabled since only one " + object_descriptions.at(type).name
                                           + " is selected");
        apply_all_button->set_sensitive(false);
    }
}

Gtk::Widget *PropertyEditorBool::create_editor()
{
    sw = Gtk::manage(new Gtk::Switch());
    connections.push_back(sw->property_active().signal_changed().connect([this] { s_signal_changed.emit(); }));
    return sw;
}

void PropertyEditorBool::reload()
{
    ScopedBlock block(connections);
    sw->set_active(value.value);
}

PropertyValue &PropertyEditorBool::get_value()
{
    value.value = sw->get_active();
    return value;
}

Gtk::Widget *PropertyEditorString::create_editor()
{
    en = Gtk::manage(new Gtk::Entry());
    en->set_alignment(1);
    connections.push_back(en->signal_changed().connect(sigc::mem_fun(*this, &PropertyEditorString::changed)));
    connections.push_back(en->signal_activate().connect(sigc::mem_fun(*this, &PropertyEditorString::activate)));
    en->signal_focus_out_event().connect(sigc::mem_fun(*this, &PropertyEditorString::focus_out_event));
    return en;
}

void PropertyEditorString::activate()
{
    if (!modified) {
        s_signal_activate.emit();
        return;
    }
    modified = false;
    s_signal_changed.emit();
    s_signal_activate.emit();
}

bool PropertyEditorString::focus_out_event(GdkEventFocus *e)
{
    activate();
    return false;
}

void PropertyEditorString::reload()
{
    ScopedBlock block(connections);
    en->set_text(value.value);
    modified = false;
}

PropertyValue &PropertyEditorString::get_value()
{
    value.value = en->get_text();
    return value;
}

void PropertyEditorString::changed()
{
    modified = true;
}

Gtk::Widget *PropertyEditorDim::create_editor()
{
    sp = Gtk::manage(new SpinButtonDim);
    sp->signal_activate().connect([this] { s_signal_activate.emit(); });
    sp->set_range(range.first, range.second);
    connections.push_back(sp->signal_value_changed().connect([this] { s_signal_changed.emit(); }));
    return sp;
}

void PropertyEditorDim::reload()
{
    ScopedBlock block(connections);
    sp->set_value(value.value);
}

PropertyValue &PropertyEditorDim::get_value()
{
    value.value = sp->get_value_as_int();
    return value;
}

void PropertyEditorDim::set_range(int64_t min, int64_t max)
{
    range = {min, max};
}

Gtk::Widget *PropertyEditorEnum::create_editor()
{
    combo = Gtk::manage(new GenericComboBox<int>);
    combo->get_cr_text().property_ellipsize() = Pango::ELLIPSIZE_END;
    combo->get_cr_text().property_width_chars() = 10;
    for (const auto &it : property.enum_items) {
        combo->append(it.first, it.second);
    }
    connections.push_back(combo->signal_changed().connect(sigc::mem_fun(*this, &PropertyEditorEnum::changed)));
    return combo;
}

void PropertyEditorEnum::reload()
{
    ScopedBlock block(connections);
    combo->set_active_key(value.value);
}

void PropertyEditorEnum::changed()
{
    s_signal_changed.emit();
}

PropertyValue &PropertyEditorEnum::get_value()
{
    if (combo->get_active_row_number() != -1)
        value.value = combo->get_active_key();
    return value;
}
Gtk::Widget *PropertyEditorStringRO::create_editor()
{
    apply_all_button->set_sensitive(false);
    readonly = true;
    la = Gtk::manage(new Gtk::Label(""));
    la->set_line_wrap(true);
    la->set_xalign(1);
    la->set_selectable(true);
    return la;
}

void PropertyEditorStringRO::reload()
{
    la->set_text(value.value);
}

PropertyValue &PropertyEditorStringRO::get_value()
{
    return value;
}

Gtk::Widget *PropertyEditorNetClass::create_editor()
{
    combo = Gtk::manage(new GenericComboBox<UUID>);
    combo->get_cr_text().property_ellipsize() = Pango::ELLIPSIZE_END;
    combo->get_cr_text().property_width_chars() = 10;
    connections.push_back(combo->signal_changed().connect(sigc::mem_fun(*this, &PropertyEditorNetClass::changed)));
    return combo;
}

void PropertyEditorNetClass::reload()
{
    ScopedBlock block(connections);
    combo->remove_all();
    for (const auto &it : my_meta.net_classes) {
        combo->append(it.first, it.second);
    }
    combo->set_active_key(value.value);
}

void PropertyEditorNetClass::changed()
{
    s_signal_changed.emit();
}

PropertyValue &PropertyEditorNetClass::get_value()
{
    if (combo->get_active_row_number() != -1)
        value.value = combo->get_active_key();
    return value;
}

Gtk::Widget *PropertyEditorLayer::create_editor()
{
    combo = Gtk::manage(new LayerComboBox);
    connections.push_back(combo->signal_changed().connect(sigc::mem_fun(*this, &PropertyEditorLayer::changed)));
    return combo;
}

void PropertyEditorLayer::reload()
{
    ScopedBlock block(connections);
    combo->remove_all();
    for (const auto &it : my_meta.layers) {
        if (!copper_only || it.second.copper)
            combo->prepend(it.second);
    }
    combo->set_active_layer(value.value);
}

void PropertyEditorLayer::changed()
{
    s_signal_changed.emit();
}

PropertyValue &PropertyEditorLayer::get_value()
{
    const auto active_layer = combo->get_active_layer();
    if (active_layer != 10000)
        value.value = active_layer;
    return value;
}

Gtk::Widget *PropertyEditorAngle::create_editor()
{
    sp = Gtk::manage(new SpinButtonAngle);
    sp->set_width_chars(7);
    sp->signal_activate().connect([this] { s_signal_activate.emit(); });
    connections.push_back(sp->signal_value_changed().connect([this] { s_signal_changed.emit(); }));
    return sp;
}

void PropertyEditorAngle::reload()
{
    ScopedBlock block(connections);
    sp->set_value(value.value);
}

PropertyValue &PropertyEditorAngle::get_value()
{
    value.value = sp->get_value_as_int();
    return value;
}


Gtk::Widget *PropertyEditorStringMultiline::create_editor()
{
    editor = Gtk::manage(new TextEditor(TextEditor::Lines::MULTI));
    connections.push_back(
            editor->signal_changed().connect(sigc::mem_fun(*this, &PropertyEditorStringMultiline::changed)));
    connections.push_back(
            editor->signal_activate().connect(sigc::mem_fun(*this, &PropertyEditorStringMultiline::activate)));
    connections.push_back(
            editor->signal_lost_focus().connect(sigc::mem_fun(*this, &PropertyEditorStringMultiline::activate)));
    return editor;
}

void PropertyEditorStringMultiline::activate()
{
    if (!modified) {
        s_signal_activate.emit();
        return;
    }
    modified = false;
    s_signal_changed.emit();
    s_signal_activate.emit();
}

void PropertyEditorStringMultiline::reload()
{
    ScopedBlock block(connections);
    editor->set_text(value.value, TextEditor::Select::NO);
    modified = false;
}

PropertyValue &PropertyEditorStringMultiline::get_value()
{
    value.value = editor->get_text();
    return value;
}

void PropertyEditorStringMultiline::changed()
{
    modified = true;
}

void PropertyEditorStringMultiline::construct()
{
    auto *ed = create_editor();
    pack_start(*ed, false, false, 0);
}


Gtk::Widget *PropertyEditorInt::create_editor()
{
    sp = Gtk::manage(new Gtk::SpinButton());
    sp->set_range(0, 65536);
    sp->set_width_chars(7);
    connections.push_back(sp->signal_value_changed().connect([this] { s_signal_changed.emit(); }));
    sp->signal_activate().connect([this] { s_signal_activate.emit(); });
    return sp;
}

void PropertyEditorInt::reload()
{
    ScopedBlock block(connections);
    sp->set_value(value.value);
}

PropertyValue &PropertyEditorInt::get_value()
{
    value.value = sp->get_value_as_int();
    return value;
}

Gtk::Widget *PropertyEditorExpand::create_editor()
{
    PropertyEditorInt::create_editor();
    sp->set_range(0, 100);
    sp->set_increments(1, 1);
    return sp;
}

Gtk::Widget *PropertyEditorPriority::create_editor()
{
    PropertyEditorInt::create_editor();
    sp->set_range(0, 100);
    sp->set_increments(1, 1);
    return sp;
}

Gtk::Widget *PropertyEditorDouble::create_editor()
{
    sp = Gtk::manage(new Gtk::SpinButton());
    sp->set_range(0, 1);
    sp->set_width_chars(7);
    connections.push_back(sp->signal_value_changed().connect(sigc::mem_fun(*this, &PropertyEditorDouble::changed)));
    return sp;
}

void PropertyEditorDouble::reload()
{
    ScopedBlock block(connections);
    sp->set_value(value.value);
}

void PropertyEditorDouble::changed()
{
    s_signal_changed.emit();
}

PropertyValue &PropertyEditorDouble::get_value()
{
    value.value = sp->get_value();
    return value;
}

Gtk::Widget *PropertyEditorOpacity::create_editor()
{
    PropertyEditorDouble::create_editor();
    ScopedBlock block(connections);
    sp->set_range(.1, 1);
    sp->set_increments(.1, .1);
    sp->set_digits(1);
    return sp;
}

Gtk::Widget *PropertyEditorScale::create_editor()
{
    PropertyEditorDouble::create_editor();
    ScopedBlock block(connections);
    sp->set_range(.1, 10);
    sp->set_increments(.1, .1);
    sp->set_digits(3);
    return sp;
}


Gtk::Widget *PropertyEditorLayerRange::create_editor()
{
    combo_start = Gtk::manage(new LayerComboBox);
    combo_end = Gtk::manage(new LayerComboBox);
    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0));
    box->get_style_context()->add_class("linked");
    box->pack_start(*combo_end, false, false, 0);
    box->pack_start(*combo_start, false, false, 0);

    connections.push_back(
            combo_start->signal_changed().connect(sigc::mem_fun(*this, &PropertyEditorLayerRange::changed)));
    connections.push_back(
            combo_end->signal_changed().connect(sigc::mem_fun(*this, &PropertyEditorLayerRange::changed)));
    return box;
}

void PropertyEditorLayerRange::reload()
{
    ScopedBlock block(connections);
    combo_start->remove_all();
    combo_end->remove_all();
    for (const auto &it : my_meta.layers) {
        if (it.second.copper) {
            combo_start->prepend(it.second);
            combo_end->prepend(it.second);
        }
    }
    combo_start->set_active_layer(value.value.start());
    combo_end->set_active_layer(value.value.end());
    update();
}

void PropertyEditorLayerRange::changed()
{
    {
        ScopedBlock block(connections);
        update();
    }
    s_signal_changed.emit();
}

void PropertyEditorLayerRange::update()
{
    combo_start->set_layer_insensitive(combo_end->get_active_layer());
    combo_end->set_layer_insensitive(combo_start->get_active_layer());
    const LayerRange r{combo_start->get_active_layer(), combo_end->get_active_layer()};
    combo_start->set_active_layer(r.start());
    combo_end->set_active_layer(r.end());
}

PropertyValue &PropertyEditorLayerRange::get_value()
{
    value.value = {combo_start->get_active_layer(), combo_end->get_active_layer()};
    // const auto active_layer = combo->get_active_layer();
    // if (active_layer != 10000)
    //     value.value = active_layer;
    return value;
}

} // namespace horizon
