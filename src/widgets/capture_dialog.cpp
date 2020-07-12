#include "capture_dialog.hpp"

namespace horizon {

CaptureDialog::CaptureDialog(Gtk::Window *parent)
    : Gtk::Dialog("Capture key sequence", *parent, Gtk::DIALOG_MODAL | Gtk::DIALOG_USE_HEADER_BAR)
{
    add_button("OK", Gtk::RESPONSE_OK);
    add_button("Cancel", Gtk::RESPONSE_CANCEL);
    set_default_response(Gtk::RESPONSE_OK);

    auto capture_box = Gtk::manage(new Gtk::EventBox);
    capture_label = Gtk::manage(new Gtk::Label("type here"));
    capture_box->add(*capture_label);

    capture_box->add_events(Gdk::KEY_PRESS_MASK | Gdk::FOCUS_CHANGE_MASK | Gdk::BUTTON_PRESS_MASK);
    capture_box->set_can_focus(true);

    capture_box->signal_button_press_event().connect([capture_box](GdkEventButton *ev) {
        capture_box->grab_focus();
        return true;
    });

    capture_box->signal_key_press_event().connect([this](GdkEventKey *ev) {
        if (!ev->is_modifier) {
            auto display = get_display()->gobj();
            auto hw_keycode = ev->hardware_keycode;
            auto state = static_cast<GdkModifierType>(ev->state);
            auto group = ev->group;
            guint keyval;
            GdkModifierType consumed_modifiers;
            if (gdk_keymap_translate_keyboard_state(gdk_keymap_get_for_display(display), hw_keycode, state, group,
                                                    &keyval, NULL, NULL, &consumed_modifiers)) {
                auto mod = state & (~consumed_modifiers);
                mod &= (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK);
                keys.emplace_back(keyval, static_cast<GdkModifierType>(mod));
            }
            update();
        }
        return true;
    });

    capture_box->signal_focus_in_event().connect([this](GdkEventFocus *ev) {
        update();
        return true;
    });

    capture_box->signal_focus_out_event().connect([this](GdkEventFocus *ev) {
        capture_label->set_markup("<i>focus to capture keys</i>");
        return true;
    });

    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 10));
    box->property_margin() = 10;
    box->pack_start(*capture_box, true, true, 0);

    auto reset_button = Gtk::manage(new Gtk::Button("Start over"));
    box->pack_start(*reset_button, true, true, 0);
    reset_button->signal_clicked().connect([this, capture_box] {
        keys.clear();
        capture_box->grab_focus();
    });


    get_content_area()->pack_start(*box, true, true, 0);
    box->show_all();
}

void CaptureDialog::update()
{
    auto txt = key_sequence_to_string(keys);
    if (txt.size() == 0) {
        txt = "type here";
    }
    capture_label->set_text(txt);
}
} // namespace horizon
