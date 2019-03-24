#include "property_editor.hpp"
#include "common/object_descr.hpp"
#include "property_panel.hpp"
#include "property_panels.hpp"
#include "block/block.hpp"
#include "core/core_schematic.hpp"
#include "core/core_board.hpp"
#include "widgets/spin_button_dim.hpp"
#include "util/util.hpp"
#include <algorithm>
#include <iostream>
#include <iomanip>

namespace horizon {

class ScopedBlock {
public:
    ScopedBlock(std::deque<sigc::connection> &conns) : connections(conns)
    {
        for (auto &conn : connections) {
            conn.block();
        }
    }

    ~ScopedBlock()
    {
        for (auto &conn : connections) {
            conn.unblock();
        }
    }

private:
    std::deque<sigc::connection> &connections;
};

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
    if (!modified)
        return;
    modified = false;
    std::string txt = en->get_text();
    s_signal_changed.emit();
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
    combo = Gtk::manage(new Gtk::ComboBoxText());
    for (const auto &it : property.enum_items) {
        combo->insert(-1, std::to_string(it.first), it.second);
    }
    connections.push_back(combo->signal_changed().connect(sigc::mem_fun(*this, &PropertyEditorEnum::changed)));
    return combo;
}

void PropertyEditorEnum::reload()
{
    ScopedBlock block(connections);
    combo->set_active_id(std::to_string(value.value));
}

void PropertyEditorEnum::changed()
{
    s_signal_changed.emit();
}

PropertyValue &PropertyEditorEnum::get_value()
{
    auto active_id = combo->get_active_id();
    if (active_id.size())
        value.value = std::stoi(active_id);
    return value;
}

Gtk::Widget *PropertyEditorStringRO::create_editor()
{
    apply_all_button->set_sensitive(false);
    readonly = true;
    la = Gtk::manage(new Gtk::Label(""));
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
    combo = Gtk::manage(new Gtk::ComboBoxText());
    connections.push_back(combo->signal_changed().connect(sigc::mem_fun(*this, &PropertyEditorNetClass::changed)));
    return combo;
}

void PropertyEditorNetClass::reload()
{
    ScopedBlock block(connections);
    combo->remove_all();
    for (const auto &it : my_meta.net_classes) {
        combo->append((std::string)it.first, it.second);
    }
    combo->set_active_id((std::string)value.value);
}

void PropertyEditorNetClass::changed()
{
    s_signal_changed.emit();
}

PropertyValue &PropertyEditorNetClass::get_value()
{
    auto active_id = combo->get_active_id();
    if (active_id.size())
        value.value = UUID(active_id);
    return value;
}

Gtk::Widget *PropertyEditorLayer::create_editor()
{
    combo = Gtk::manage(new Gtk::ComboBoxText());
    connections.push_back(combo->signal_changed().connect(sigc::mem_fun(*this, &PropertyEditorLayer::changed)));
    return combo;
}

void PropertyEditorLayer::reload()
{
    ScopedBlock block(connections);
    combo->remove_all();
    for (const auto &it : my_meta.layers) {
        if (!copper_only || it.second.copper)
            combo->insert(0, std::to_string(it.first), it.second.name);
    }
    combo->set_active_id(std::to_string(value.value));
}

void PropertyEditorLayer::changed()
{
    s_signal_changed.emit();
}

PropertyValue &PropertyEditorLayer::get_value()
{
    auto active_id = combo->get_active_id();
    if (active_id.size())
        value.value = std::stoi(active_id);
    return value;
}

Gtk::Widget *PropertyEditorAngle::create_editor()
{
    sp = Gtk::manage(new Gtk::SpinButton());
    sp->set_range(0, 65536);
    sp->set_wrap(true);
    sp->set_width_chars(7);
    sp->set_increments(4096, 4096);
    sp->signal_output().connect(sigc::mem_fun(*this, &PropertyEditorAngle::sp_output));
    sp->signal_input().connect(sigc::mem_fun(*this, &PropertyEditorAngle::sp_input));
    connections.push_back(sp->signal_value_changed().connect(sigc::mem_fun(*this, &PropertyEditorAngle::changed)));
    return sp;
}

void PropertyEditorAngle::reload()
{
    ScopedBlock block(connections);
    sp->set_value(value.value);
}

bool PropertyEditorAngle::sp_output()
{
    auto adj = sp->get_adjustment();
    double v = adj->get_value();

    std::stringstream stream;
    stream.imbue(get_locale());
    stream << std::fixed << std::setprecision(2) << (v / 65536.0) * 360 << "°";

    sp->set_text(stream.str());
    return true;
}

int PropertyEditorAngle::sp_input(double *v)
{
    auto txt = sp->get_text();
    int64_t va = 0;
    try {
        va = (std::stod(txt) / 360.0) * 65536;
        *v = va;
    }
    catch (const std::exception &e) {
        return false;
    }


    return true;
}

void PropertyEditorAngle::changed()
{
    s_signal_changed.emit();
}

PropertyValue &PropertyEditorAngle::get_value()
{
    value.value = sp->get_value_as_int();
    return value;
}


Gtk::Widget *PropertyEditorStringMultiline::create_editor()
{
    en = Gtk::manage(new Gtk::TextView());
    en->set_left_margin(3);
    en->set_right_margin(3);
    en->set_top_margin(3);
    en->set_bottom_margin(3);
    connections.push_back(
            en->get_buffer()->signal_changed().connect(sigc::mem_fun(*this, &PropertyEditorStringMultiline::changed)));
    en->signal_focus_out_event().connect(sigc::mem_fun(*this, &PropertyEditorStringMultiline::focus_out_event));
    auto sc = Gtk::manage(new Gtk::ScrolledWindow());
    sc->set_shadow_type(Gtk::SHADOW_IN);
    sc->add(*en);
    en->show();
    return sc;
}

void PropertyEditorStringMultiline::activate()
{
    if (!modified)
        return;
    modified = false;
    s_signal_changed.emit();
}

bool PropertyEditorStringMultiline::focus_out_event(GdkEventFocus *e)
{
    activate();
    return false;
}

void PropertyEditorStringMultiline::reload()
{
    ScopedBlock block(connections);
    en->get_buffer()->set_text(value.value);
    modified = false;
}

PropertyValue &PropertyEditorStringMultiline::get_value()
{
    value.value = en->get_buffer()->get_text();
    return value;
}

void PropertyEditorStringMultiline::changed()
{
    modified = true;
    s_signal_changed.emit();
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
    connections.push_back(sp->signal_value_changed().connect(sigc::mem_fun(*this, &PropertyEditorInt::changed)));
    return sp;
}

void PropertyEditorInt::reload()
{
    ScopedBlock block(connections);
    sp->set_value(value.value);
}

void PropertyEditorInt::changed()
{
    s_signal_changed.emit();
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

} // namespace horizon
