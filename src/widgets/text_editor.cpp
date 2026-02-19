#include "text_editor.hpp"
#include "util/gtk_util.hpp"

namespace horizon {

TextEditor::TextEditor(Lines mode, ShowHints show_hints)
{
    entry = Gtk::manage(new Gtk::Entry());
    entry->signal_activate().connect([this] { s_signal_activate.emit(); });
    entry->signal_changed().connect([this] { s_signal_changed.emit(); });
    entry_focus_out_conn = entry->signal_focus_out_event().connect([this](GdkEventFocus *ev) -> bool {
        s_signal_lost_focus.emit();
        return false;
    });
    if (mode == Lines::MULTI) {
        entry->signal_key_press_event().connect([this](GdkEventKey *ev) {
            if (ev->keyval == GDK_KEY_Return && (ev->state & Gdk::SHIFT_MASK)) {
                entry_focus_out_conn.disconnect();
                auto txt = entry->get_text();
                auto pos = entry->get_position();
                txt.insert(pos, "\n");
                set_transition_duration(100);
                set_text(txt, Select::NO);
                auto iter = view->get_buffer()->begin();
                iter.forward_cursor_positions(pos + 1);
                view->get_buffer()->place_cursor(iter);
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
    view->get_buffer()->signal_changed().connect([this] { s_signal_changed.emit(); });
    view->signal_key_press_event().connect(
            [this](GdkEventKey *ev) {
                if (ev->keyval == GDK_KEY_Return && (ev->state & Gdk::SHIFT_MASK)) {
                    s_signal_activate.emit();
                    return true;
                }
                return false;
            },
            false);
    view->signal_focus_out_event().connect([this](GdkEventFocus *ev) {
        s_signal_lost_focus.emit();
        return false;
    });
    sc->add(*view);


    set_homogeneous(false);
    set_interpolate_size(true);
    set_transition_duration(0);
    set_transition_type(Gtk::STACK_TRANSITION_TYPE_CROSSFADE);
    {
        auto lbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 2));
        lbox->pack_start(*entry, true, true, 0);
        if (mode == Lines::MULTI && show_hints == ShowHints::YES) {
            auto la = Gtk::manage(new Gtk::Label("Press Shift+Enter to insert a line break"));
            la->get_style_context()->add_class("dim-label");
            la->set_xalign(0);
            make_label_small(la);
            lbox->pack_start(*la, false, false, 0);
        }
        add(*lbox, "entry");
        lbox->show_all();
    }
    {
        auto lbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 2));
        lbox->pack_start(*sc, true, true, 0);
        if (show_hints == ShowHints::YES) {
            auto la = Gtk::manage(new Gtk::Label("Press Shift+Enter to finish editing"));
            la->get_style_context()->add_class("dim-label");
            la->set_xalign(0);
            make_label_small(la);
            lbox->pack_start(*la, false, false, 0);
        }
        add(*lbox, "view");
        lbox->show_all();
    }

    set_visible_child("entry");
}

void TextEditor::set_text(const std::string &text, Select select)
{
    if (text.find('\n') != std::string::npos) {
        view->get_buffer()->set_text(text);
        if (select == Select::YES)
            view->grab_focus();
        // view->get_buffer()->select_range(view->get_buffer()->begin(), view->get_buffer()->end());
        set_visible_child("view");
    }
    else {
        entry->set_text(text);
        if (select == Select::YES)
            entry->select_region(0, -1);
        set_visible_child("entry");
    }
}

std::string TextEditor::get_text() const
{
    if (get_visible_child_name() == "entry")
        return entry->get_text();
    else
        return view->get_buffer()->get_text();
}

bool TextEditor::is_multi_line() const
{
    return get_visible_child_name() == "view";
}

} // namespace horizon
