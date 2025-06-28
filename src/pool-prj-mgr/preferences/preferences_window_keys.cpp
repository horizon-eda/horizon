#include "preferences_window_keys.hpp"
#include "preferences/preferences.hpp"
#include "util/gtk_util.hpp"
#include "util/util.hpp"
#include <nlohmann/json.hpp>
#include "core/tool_id.hpp"
#include "action_editor.hpp"
#include "imp/actions.hpp"

namespace horizon {
class ActionEditor : public ActionEditorBase {
public:
    ActionEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, Preferences &prefs, ActionToolID act,
                 ActionCatalogItem::Availability av, const std::string &title)
        : ActionEditorBase(cobject, x, prefs, title), action(act), availability(av)
    {
        update();
        set_placeholder_text("use default");
    }
    static ActionEditor *create(Preferences &prefs, ActionToolID action, ActionCatalogItem::Availability availability,
                                const std::string &title)
    {
        ActionEditor *w;
        Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
        x->add_from_resource("/org/horizon-eda/horizon/pool-prj-mgr/preferences/preferences.ui", "action_editor");
        x->get_widget_derived("action_editor", w, prefs, action, availability, title);
        w->reference();
        return w;
    }

private:
    const ActionToolID action;
    const ActionCatalogItem::Availability availability;

    std::vector<KeySequence> *maybe_get_keys() override
    {
        if (preferences.key_sequences.keys.count(action)
            && preferences.key_sequences.keys.at(action).count(availability)) {
            return &preferences.key_sequences.keys.at(action).at(availability);
        }
        else {
            return nullptr;
        }
    }
    std::vector<KeySequence> &get_keys() override
    {
        return preferences.key_sequences.keys[action][availability];
    }
};

KeySequencesPreferencesEditor::KeySequencesPreferencesEditor(BaseObjectType *cobject,
                                                             const Glib::RefPtr<Gtk::Builder> &x, Preferences &prefs)
    : Gtk::Grid(cobject), preferences(prefs), keyseq_preferences(preferences.key_sequences)
{
    GET_WIDGET(key_sequences_treeview);
    GET_WIDGET(action_editors);
    key_sequences_store = Gtk::TreeStore::create(tree_columns);
    key_sequences_treeview->set_model(key_sequences_store);
    tree_view_set_search_contains(key_sequences_treeview);

    for (const auto &it_gr : action_group_catalog) {
        Gtk::TreeModel::Row gr_row = *key_sequences_store->append();
        gr_row[tree_columns.name] = it_gr.second;
        gr_row[tree_columns.action] = ActionToolID();
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
            sigc::mem_fun(*this, &KeySequencesPreferencesEditor::update_action_editors));

    key_sequences_treeview->expand_all();

    Gtk::Button *save_button;
    x->get_widget("key_sequences_save_button", save_button);
    save_button->signal_clicked().connect(sigc::mem_fun(*this, &KeySequencesPreferencesEditor::handle_save));

    Gtk::Button *load_button;
    x->get_widget("key_sequences_load_button", load_button);
    load_button->signal_clicked().connect(sigc::mem_fun(*this, &KeySequencesPreferencesEditor::handle_load));

    Gtk::Button *load_default_button;
    x->get_widget("key_sequences_load_default_button", load_default_button);
    load_default_button->signal_clicked().connect(
            sigc::mem_fun(*this, &KeySequencesPreferencesEditor::handle_load_default));

    signal_key_press_event().connect([this](const GdkEventKey *ev) {
        if (ev->keyval == GDK_KEY_C && (ev->state & GDK_CONTROL_MASK)) {
            std::ostringstream oss;
            oss << "|Action | Key sequence | Comments|\n";
            oss << "|-|-|-|\n";
            for (const auto &it : keyseq_preferences.keys) {
                for (const auto &it2 : it.second) {
                    for (const auto &it3 : it2.second) {
                        oss << "|" << action_catalog.at(it.first).name << "|`" << key_sequence_to_string(it3)
                            << "` | |\n";
                    }
                }
            }
            Gtk::Clipboard::get()->set_text(oss.str());
            return true;
        }
        else {
            return false;
        }
    });

    update_action_editors();
}

static const std::map<ActionCatalogItem::Availability, std::string> availabilities = {
        {ActionCatalogItem::AVAILABLE_IN_BOARD, "Board"},     {ActionCatalogItem::AVAILABLE_IN_SCHEMATIC, "Schematic"},
        {ActionCatalogItem::AVAILABLE_IN_SYMBOL, "Symbol"},   {ActionCatalogItem::AVAILABLE_IN_PADSTACK, "Padstack"},
        {ActionCatalogItem::AVAILABLE_IN_PACKAGE, "Package"}, {ActionCatalogItem::AVAILABLE_IN_FRAME, "Frame"},
        {ActionCatalogItem::AVAILABLE_IN_DECAL, "Decal"},     {ActionCatalogItem::AVAILABLE_IN_3D, "3D Preview"}};

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
        ActionToolID action = row[tree_columns.action];
        if (action.is_valid()) {
            const auto cat = action_catalog.at(row[tree_columns.action]);
            auto av = static_cast<unsigned int>(cat.availability);
            int count = 0;
            while (av) {
                count += av & 1;
                av >>= 1;
            }
            std::vector<ActionEditor *> eds;
            if (count > 1) {
                auto ed = Gtk::manage(
                        ActionEditor::create(preferences, action, ActionCatalogItem::AVAILABLE_EVERYWHERE, "Default"));
                eds.push_back(ed);
            }
            for (const auto &it_av : availabilities) {
                if (cat.availability & it_av.first) {
                    auto ed = Gtk::manage(ActionEditor::create(preferences, action, it_av.first, it_av.second));
                    eds.push_back(ed);
                }
            }
            for (auto ed : eds) {
                action_editors->add(*ed);
                ed->show();
                ed->signal_changed().connect(sigc::mem_fun(*this, &KeySequencesPreferencesEditor::update_keys));
                ed->unreference();
            }
        }
    }
}

void KeySequencesPreferencesEditor::update_keys()
{
    for (auto &it : key_sequences_store->children()) {
        for (auto &it2 : it->children()) {
            if (keyseq_preferences.keys.count(it2[tree_columns.action])) {
                std::stringstream s;
                for (const auto &it_seq : keyseq_preferences.keys.at(it2[tree_columns.action])) {
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

void KeySequencesPreferencesEditor::handle_save()
{
    auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));
    std::string filename;
    {

        GtkFileChooserNative *native = gtk_file_chooser_native_new("Save keybindings", GTK_WINDOW(top->gobj()),
                                                                   GTK_FILE_CHOOSER_ACTION_SAVE, "_Save", "_Cancel");
        auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
        chooser->set_do_overwrite_confirmation(true);
        chooser->set_current_name("keys.json");

        if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(native)) == GTK_RESPONSE_ACCEPT) {
            filename = chooser->get_filename();
            if (!endswith(filename, ".json"))
                filename += ".json";
        }
    }
    if (filename.size()) {
        while (1) {
            std::string error_str;
            try {
                auto j = keyseq_preferences.serialize();
                save_json_to_file(filename, j);
                break;
            }
            catch (const std::exception &e) {
                error_str = e.what();
            }
            catch (...) {
                error_str = "unknown error";
            }
            if (error_str.size()) {
                Gtk::MessageDialog dia(*top, "Export error", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_NONE);
                dia.set_secondary_text(error_str);
                dia.add_button("Cancel", Gtk::RESPONSE_CANCEL);
                dia.add_button("Retry", 1);
                if (dia.run() != 1)
                    break;
            }
        }
    }
}


void KeySequencesPreferencesEditor::handle_load()
{
    auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));
    GtkFileChooserNative *native = gtk_file_chooser_native_new("Open keybindings", GTK_WINDOW(top->gobj()),
                                                               GTK_FILE_CHOOSER_ACTION_OPEN, "_Open", "_Cancel");
    auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
    auto filter = Gtk::FileFilter::create();
    filter->set_name("Horizon key bindings");
    filter->add_pattern("*.json");
    chooser->add_filter(filter);

    if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(native)) == GTK_RESPONSE_ACCEPT) {
        auto filename = chooser->get_filename();
        std::string error_str;
        try {
            auto j = load_json_from_file(filename);
            keyseq_preferences.load_from_json(j);
            update_action_editors();
            update_keys();
            preferences.signal_changed().emit();
        }
        catch (const std::exception &e) {
            error_str = e.what();
        }
        catch (...) {
            error_str = "unknown error";
        }
        if (error_str.size()) {
            Gtk::MessageDialog dia(*top, "Import error", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
            dia.set_secondary_text(error_str);
            dia.run();
        }
    }
}

void KeySequencesPreferencesEditor::handle_load_default()
{
    keyseq_preferences.load_from_json(json_from_resource("/org/horizon-eda/horizon/imp/keys_default.json"));
    update_action_editors();
    update_keys();
    preferences.signal_changed().emit();
}

KeySequencesPreferencesEditor *KeySequencesPreferencesEditor::create(Preferences &prefs)
{
    KeySequencesPreferencesEditor *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/horizon-eda/horizon/pool-prj-mgr/preferences/preferences.ui", "key_sequences_box");
    x->get_widget_derived("key_sequences_box", w, prefs);
    w->reference();
    return w;
}

} // namespace horizon
