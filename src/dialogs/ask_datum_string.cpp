#include "ask_datum_string.hpp"
#include "util/gtk_util.hpp"
#include <iostream>

namespace horizon {


AskDatumStringDialog::AskDatumStringDialog(Gtk::Window *parent, const std::string &label, bool multiline)
    : Gtk::Dialog("Enter Datum", *parent, Gtk::DialogFlags::DIALOG_MODAL | Gtk::DialogFlags::DIALOG_USE_HEADER_BAR)
{
    add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
    add_button("OK", Gtk::ResponseType::RESPONSE_OK);

    set_default_response(Gtk::ResponseType::RESPONSE_OK);


    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 4));
    box->property_margin() = 6;
    {
        auto la = Gtk::manage(new Gtk::Label(label));
        la->set_halign(Gtk::ALIGN_START);
        box->pack_start(*la, false, false, 0);
    }
    entry = Gtk::manage(new Gtk::Entry());
    entry->signal_activate().connect([this] { response(Gtk::RESPONSE_OK); });
    if (multiline) {
        entry->signal_key_press_event().connect([this](GdkEventKey *ev) {
            if (ev->keyval == GDK_KEY_Return && (ev->state & Gdk::SHIFT_MASK)) {
                auto txt = get_text();
                stack->set_transition_duration(100);
                stack->set_visible_child("view");
                set_text(txt + "\n");
                view->get_buffer()->place_cursor(view->get_buffer()->end());
                return true;
            }
            return false;
        });
    }

    auto sc = Gtk::manage(new Gtk::ScrolledWindow);
    sc->set_shadow_type(Gtk::SHADOW_IN);
    sc->set_min_content_height(100);
    view = Gtk::manage(new Gtk::TextView);
    view->set_top_margin(4);
    view->set_bottom_margin(4);
    view->set_left_margin(4);
    view->set_right_margin(4);
    view->signal_key_press_event().connect(
            [this](GdkEventKey *ev) {
                if (ev->keyval == GDK_KEY_Return && (ev->state & Gdk::SHIFT_MASK)) {
                    response(Gtk::RESPONSE_OK);
                    return true;
                }
                return false;
            },
            false);
    sc->add(*view);

    stack = Gtk::manage(new Gtk::Stack);
    stack->set_homogeneous(false);
    stack->set_interpolate_size(true);
    stack->set_transition_duration(0);
    stack->set_transition_type(Gtk::STACK_TRANSITION_TYPE_CROSSFADE);
    {
        auto lbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 2));
        lbox->pack_start(*entry, true, true, 0);
        if (multiline) {
            auto la = Gtk::manage(new Gtk::Label("Press Shift+Enter to insert a line break"));
            la->get_style_context()->add_class("dim-label");
            la->set_xalign(0);
            make_label_small(la);
            lbox->pack_start(*la, false, false, 0);
        }
        stack->add(*lbox, "entry");
    }
    {
        auto lbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 2));
        lbox->pack_start(*sc, true, true, 0);
        auto la = Gtk::manage(new Gtk::Label("Press Shift+Enter to finish editing"));
        la->get_style_context()->add_class("dim-label");
        la->set_xalign(0);
        make_label_small(la);
        lbox->pack_start(*la, false, false, 0);
        stack->add(*lbox, "view");
    }

    stack->set_visible_child("entry");

    box->pack_start(*stack, true, true, 0);
    get_content_area()->set_border_width(0);
    get_content_area()->pack_start(*box, true, true, 0);
    show_all();
}

void AskDatumStringDialog::set_text(const std::string &text)
{
    if (text.find('\n') != std::string::npos) {
        view->get_buffer()->set_text(text);
        view->grab_focus();
        // view->get_buffer()->select_range(view->get_buffer()->begin(), view->get_buffer()->end());
        stack->set_visible_child("view");
    }
    else {
        entry->set_text(Glib::strescape(text));
        entry->select_region(0, -1);
        stack->set_visible_child("entry");
    }
}
std::string AskDatumStringDialog::get_text()
{
    if (stack->get_visible_child_name() == "entry")
        return Glib::strcompress(entry->get_text());
    else
        return view->get_buffer()->get_text();
}

} // namespace horizon
