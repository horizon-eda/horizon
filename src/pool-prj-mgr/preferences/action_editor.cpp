#include "action_editor.hpp"
#include "widgets/capture_dialog.hpp"
#include "preferences/preferences.hpp"
#include "util/gtk_util.hpp"

namespace horizon {

class MyBox : public Gtk::Box {
public:
    MyBox(KeySequence &k) : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 5), keys(k)
    {
        property_margin() = 5;
    }
    KeySequence &keys;
};


ActionEditorBase::ActionEditorBase(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, Preferences &prefs,
                                   const std::string &title)
    : Gtk::Box(cobject), preferences(prefs)
{
    Gtk::Label *la;
    x->get_widget("action_label", la);
    la->set_text(title);

    Gtk::EventBox *key_sequence_add_box;
    x->get_widget("key_sequence_add_box", key_sequence_add_box);
    key_sequence_add_box->add_events(Gdk::BUTTON_PRESS_MASK);

    key_sequence_add_box->signal_button_press_event().connect([this](GdkEventButton *ev) {
        auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));
        CaptureDialog dia(top);
        if (dia.run() == Gtk::RESPONSE_OK) {
            if (dia.keys.size()) {
                get_keys().push_back(dia.keys);
                update();
                preferences.signal_changed().emit();
            }
        }
        return false;
    });

    GET_WIDGET(action_listbox);
    {
        placeholder_label = Gtk::manage(new Gtk::Label());
        placeholder_label->property_margin() = 10;
        placeholder_label->get_style_context()->add_class("dim-label");
        action_listbox->set_placeholder(*placeholder_label);
        placeholder_label->show();
    }
    action_listbox->set_header_func(&header_func_separator);

    action_listbox->signal_row_activated().connect([this](Gtk::ListBoxRow *row) {
        auto my_box = dynamic_cast<MyBox *>(row->get_child());
        auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));
        CaptureDialog dia(top);
        if (dia.run() == Gtk::RESPONSE_OK) {
            if (dia.keys.size()) {
                my_box->keys = dia.keys;
                update();
                preferences.signal_changed().emit();
            }
        }
    });
}

void ActionEditorBase::update()
{
    {
        auto children = action_listbox->get_children();
        for (auto ch : children) {
            delete ch;
        }
    }
    size_t i = 0;
    auto keys = maybe_get_keys();
    if (keys) {
        for (auto &it : *keys) {
            auto box = Gtk::manage(new MyBox(it));
            box->property_margin() = 5;
            auto la = Gtk::manage(new Gtk::Label(key_sequence_to_string(it)));
            la->set_xalign(0);
            auto delete_button = Gtk::manage(new Gtk::Button());
            delete_button->set_relief(Gtk::RELIEF_NONE);
            delete_button->signal_clicked().connect([this, i, keys] {
                keys->erase(keys->begin() + i);
                update();
                preferences.signal_changed().emit();
            });
            delete_button->set_image_from_icon_name("list-remove-symbolic", Gtk::ICON_SIZE_BUTTON);
            box->pack_start(*la, true, true, 0);
            box->pack_start(*delete_button, false, false, 0);
            action_listbox->append(*box);
            box->show_all();
            i++;
        }
    }
    auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));
    if (top)
        top->queue_draw();
    s_signal_changed.emit();
}

void ActionEditorBase::set_placeholder_text(const char *t)
{
    placeholder_label->set_text(t);
}

} // namespace horizon
