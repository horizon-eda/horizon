#include "preferences_window_keys.hpp"
#include "preferences/preferences.hpp"
#include "util/gtk_util.hpp"

namespace horizon {

class ActionEditor : public Gtk::Box {
public:
    ActionEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, Preferences *prefs,
                 KeySequencesPreferences *keyseq_prefs, std::vector<KeySequence2> *keys, const std::string &title,
                 KeySequencesPreferencesEditor *parent);
    static ActionEditor *create(Preferences *prefs, KeySequencesPreferences *keyseq_prefs,
                                std::vector<KeySequence2> *keys, const std::string &title,
                                KeySequencesPreferencesEditor *parent);

private:
    std::vector<KeySequence2> *keys;
    KeySequencesPreferencesEditor *parent;

    Gtk::ListBox *action_listbox = nullptr;
    void update();
};

#define GET_WIDGET(name)                                                                                               \
    do {                                                                                                               \
        x->get_widget(#name, name);                                                                                    \
    } while (0)

KeySequencesPreferencesEditor::KeySequencesPreferencesEditor(BaseObjectType *cobject,
                                                             const Glib::RefPtr<Gtk::Builder> &x, Preferences *prefs,
                                                             KeySequencesPreferences *keyseq_prefs)
    : Gtk::Grid(cobject), preferences(prefs), keyseq_preferences(keyseq_prefs)
{
    GET_WIDGET(key_sequences_treeview);
    GET_WIDGET(action_editors);
    key_sequences_store = Gtk::TreeStore::create(tree_columns);
    key_sequences_treeview->set_model(key_sequences_store);

    for (const auto &it_gr : action_group_catalog) {
        Gtk::TreeModel::Row gr_row = *key_sequences_store->append();
        gr_row[tree_columns.name] = it_gr.second;
        gr_row[tree_columns.action] = std::make_pair(ActionID::NONE, ToolID::NONE);
        for (const auto &it_act : action_catalog) {
            if (it_act.second.group == it_gr.first
                && !(it_act.second.flags & ActionCatalogItem::FLAGS_NO_PREFERENCES)) {
                Gtk::TreeModel::Row row = *key_sequences_store->append(gr_row.children());
                row[tree_columns.name] = it_act.second.name;
                row[tree_columns.action] = it_act.first;
            }
        }
    }

    update_keys();

    key_sequences_treeview->append_column("Action", tree_columns.name);
    key_sequences_treeview->append_column("Keys", tree_columns.keys);

    key_sequences_treeview->get_selection()->signal_changed().connect(
            sigc::mem_fun(this, &KeySequencesPreferencesEditor::update_action_editors));

    key_sequences_treeview->expand_all();

    update_action_editors();
}

static const std::map<ActionCatalogItem::Availability, std::string> availabilities = {
        {ActionCatalogItem::AVAILABLE_IN_BOARD, "Board"},     {ActionCatalogItem::AVAILABLE_IN_SCHEMATIC, "Schematic"},
        {ActionCatalogItem::AVAILABLE_IN_SYMBOL, "Symbol"},   {ActionCatalogItem::AVAILABLE_IN_PADSTACK, "Padstack"},
        {ActionCatalogItem::AVAILABLE_IN_PACKAGE, "Package"},
};

void KeySequencesPreferencesEditor::update_action_editors()
{
    {
        auto children = action_editors->get_children();
        for (auto ch : children) {
            delete ch;
        }
    }
    auto it = key_sequences_treeview->get_selection()->get_selected();
    if (it) {
        Gtk::TreeModel::Row row = *it;
        std::pair<ActionID, ToolID> key = row[tree_columns.action];
        if (key.first != ActionID::NONE) {
            const auto cat = action_catalog.at(row[tree_columns.action]);
            auto av = static_cast<unsigned int>(cat.availability);
            int count = 0;
            while (av) {
                count += av & 1;
                av >>= 1;
            }
            std::vector<ActionEditor *> eds;
            if (count > 1) {
                auto ed = Gtk::manage(ActionEditor::create(
                        preferences, keyseq_preferences,
                        &keyseq_preferences->keys[key][ActionCatalogItem::AVAILABLE_EVERYWHERE], "Default", this));
                eds.push_back(ed);
            }
            for (const auto &it_av : availabilities) {
                if (cat.availability & it_av.first) {
                    auto ed = Gtk::manage(ActionEditor::create(preferences, keyseq_preferences,
                                                               &keyseq_preferences->keys[key][it_av.first],
                                                               it_av.second, this));
                    eds.push_back(ed);
                }
            }
            for (auto ed : eds) {
                action_editors->add(*ed);
                ed->show();
                ed->unreference();
            }
        }
    }
}

void KeySequencesPreferencesEditor::update_keys()
{
    for (auto &it : key_sequences_store->children()) {
        for (auto &it2 : it->children()) {
            if (keyseq_preferences->keys.count(it2[tree_columns.action])) {
                std::stringstream s;
                for (const auto &it_seq : keyseq_preferences->keys.at(it2[tree_columns.action])) {
                    const auto &seqs = it_seq.second;
                    std::transform(seqs.begin(), seqs.end(), std::ostream_iterator<std::string>(s, ", "),
                                   [](const auto &x) { return key_sequence_to_string(x); });
                }
                auto str = s.str();
                if (str.size()) {
                    str.pop_back();
                    str.pop_back();
                }
                it2[tree_columns.keys] = str;
            }
            else {
                it2[tree_columns.keys] = "";
            }
        }
    }
}

KeySequencesPreferencesEditor *KeySequencesPreferencesEditor::create(Preferences *prefs,
                                                                     KeySequencesPreferences *keyseq_prefs)
{
    KeySequencesPreferencesEditor *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/net/carrotIndustries/horizon/pool-prj-mgr/preferences.ui", "key_sequences_box");
    x->get_widget_derived("key_sequences_box", w, prefs, keyseq_prefs);
    w->reference();
    return w;
}


class CaptureDialog : public Gtk::Dialog {
public:
    CaptureDialog(Gtk::Window *parent);
    KeySequence2 keys;

private:
    Gtk::Label *capture_label = nullptr;
    void update();
};


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

class MyBox : public Gtk::Box {
public:
    MyBox(KeySequence2 &k) : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 5), keys(k)
    {
        property_margin() = 5;
    }
    KeySequence2 &keys;
};


ActionEditor::ActionEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, Preferences *prefs,
                           KeySequencesPreferences *keyseq_prefs, std::vector<KeySequence2> *k,
                           const std::string &title, KeySequencesPreferencesEditor *p)
    : Gtk::Box(cobject), keys(k), parent(p)
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
                keys->push_back(dia.keys);
                update();
            }
        }
        return false;
    });

    GET_WIDGET(action_listbox);
    {
        auto placeholder = Gtk::manage(new Gtk::Label("use default"));
        placeholder->property_margin() = 10;
        placeholder->get_style_context()->add_class("dim-label");
        action_listbox->set_placeholder(*placeholder);
        placeholder->show();
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
            }
        }
    });

    update();
}

void ActionEditor::update()
{
    {
        auto children = action_listbox->get_children();
        for (auto ch : children) {
            delete ch;
        }
    }
    size_t i = 0;
    for (auto &it : *keys) {
        auto box = Gtk::manage(new MyBox(it));
        box->property_margin() = 5;
        auto la = Gtk::manage(new Gtk::Label(key_sequence_to_string(it)));
        la->set_xalign(0);
        auto delete_button = Gtk::manage(new Gtk::Button());
        delete_button->set_relief(Gtk::RELIEF_NONE);
        delete_button->signal_clicked().connect([this, i] {
            keys->erase(keys->begin() + i);
            update();
        });
        delete_button->set_image_from_icon_name("list-remove-symbolic", Gtk::ICON_SIZE_BUTTON);
        box->pack_start(*la, true, true, 0);
        box->pack_start(*delete_button, false, false, 0);
        action_listbox->append(*box);
        box->show_all();
        i++;
    }
    auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));
    if (top)
        top->queue_draw();
    parent->update_keys();
}


ActionEditor *ActionEditor::create(Preferences *prefs, KeySequencesPreferences *keyseq_prefs,
                                   std::vector<KeySequence2> *keys, const std::string &title,
                                   KeySequencesPreferencesEditor *parent)
{
    ActionEditor *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/net/carrotIndustries/horizon/pool-prj-mgr/preferences.ui", "action_editor");
    x->get_widget_derived("action_editor", w, prefs, keyseq_prefs, keys, title, parent);
    w->reference();
    return w;
}

} // namespace horizon
