#include "preferences_window_in_tool_keys.hpp"
#include "preferences/preferences.hpp"
#include "util/gtk_util.hpp"
#include "util/util.hpp"
#include "nlohmann/json.hpp"
#include "core/tool_id.hpp"
#include "action_editor.hpp"
#include "imp/in_tool_action_catalog.hpp"
#include <set>

namespace horizon {
class InToolActionEditor : public ActionEditorBase {
public:
    InToolActionEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, Preferences &prefs,
                       InToolActionID act, const std::string &title)
        : ActionEditorBase(cobject, x, prefs, title), action(act)
    {
        update();
        set_placeholder_text("no keys defined");
    }
    static InToolActionEditor *create(Preferences &prefs, InToolActionID action, const std::string &title)
    {
        InToolActionEditor *w;
        Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
        x->add_from_resource("/org/horizon-eda/horizon/pool-prj-mgr/preferences/preferences.ui", "action_editor");
        x->get_widget_derived("action_editor", w, prefs, action, title);
        w->reference();
        return w;
    }

private:
    const InToolActionID action;

    std::vector<KeySequence> *maybe_get_keys() override
    {
        if (preferences.in_tool_key_sequences.keys.count(action)) {
            return &preferences.in_tool_key_sequences.keys.at(action);
        }
        else {
            return nullptr;
        }
    }
    std::vector<KeySequence> &get_keys() override
    {
        return preferences.in_tool_key_sequences.keys[action];
    }
};

InToolKeySequencesPreferencesEditor::InToolKeySequencesPreferencesEditor(BaseObjectType *cobject,
                                                                         const Glib::RefPtr<Gtk::Builder> &x,
                                                                         Preferences &prefs)
    : Gtk::Grid(cobject), preferences(prefs), in_tool_keyseq_preferences(preferences.in_tool_key_sequences)
{
    GET_WIDGET(key_sequences_treeview);
    GET_WIDGET(action_editors);
    key_sequences_store = Gtk::TreeStore::create(tree_columns);
    key_sequences_treeview->set_model(key_sequences_store);
    tree_view_set_search_contains(key_sequences_treeview);

    std::set<ToolID> tools;
    for (const auto &[aid, item] : in_tool_action_catalog) {
        tools.insert(item.tool);
    }

    for (const auto tool_id : tools) {
        Gtk::TreeModel::Row gr_row = *key_sequences_store->append();
        if (tool_id == ToolID::NONE)
            gr_row[tree_columns.name] = "Common";
        else
            gr_row[tree_columns.name] = action_catalog.at(tool_id).name;
        gr_row[tree_columns.action] = InToolActionID::NONE;
        gr_row[tree_columns.tool] = tool_id;
        for (const auto &[aid, item] : in_tool_action_catalog) {
            if (item.tool == tool_id && !(item.flags & InToolActionCatalogItem::FLAGS_NO_PREFERENCES)) {
                Gtk::TreeModel::Row row = *key_sequences_store->append(gr_row.children());
                row[tree_columns.name] = item.name;
                row[tree_columns.action] = aid;
                row[tree_columns.tool] = tool_id;
            }
        }
    }

    update_keys();

    key_sequences_treeview->append_column("Action", tree_columns.name);
    key_sequences_treeview->append_column("Keys", tree_columns.keys);

    key_sequences_treeview->get_selection()->signal_changed().connect(
            sigc::mem_fun(*this, &InToolKeySequencesPreferencesEditor::update_action_editors));

    key_sequences_treeview->expand_all();

    Gtk::Button *save_button;
    x->get_widget("key_sequences_save_button", save_button);
    save_button->signal_clicked().connect(sigc::mem_fun(*this, &InToolKeySequencesPreferencesEditor::handle_save));

    Gtk::Button *load_button;
    x->get_widget("key_sequences_load_button", load_button);
    load_button->signal_clicked().connect(sigc::mem_fun(*this, &InToolKeySequencesPreferencesEditor::handle_load));

    Gtk::Button *load_default_button;
    x->get_widget("key_sequences_load_default_button", load_default_button);
    load_default_button->signal_clicked().connect(
            sigc::mem_fun(*this, &InToolKeySequencesPreferencesEditor::handle_load_default));

    update_action_editors();
}

void InToolKeySequencesPreferencesEditor::update_action_editors()
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
        const InToolActionID action = row[tree_columns.action];
        const ToolID tool_id = row[tree_columns.tool];
        if (action != InToolActionID::NONE) {
            auto ed = Gtk::manage(
                    InToolActionEditor::create(preferences, action, in_tool_action_catalog.at(action).name));
            action_editors->add(*ed);
            ed->show();
            ed->signal_changed().connect(sigc::mem_fun(*this, &InToolKeySequencesPreferencesEditor::update_keys));
            ed->unreference();
        }
        else {
            for (const auto &[action_id, item] : in_tool_action_catalog) {
                if (item.tool == tool_id && !(item.flags & InToolActionCatalogItem::FLAGS_NO_PREFERENCES)) {
                    auto ed = Gtk::manage(InToolActionEditor::create(preferences, action_id, item.name));
                    action_editors->add(*ed);
                    ed->show();
                    ed->signal_changed().connect(
                            sigc::mem_fun(*this, &InToolKeySequencesPreferencesEditor::update_keys));
                    ed->unreference();
                }
            }
        }
    }
}

void InToolKeySequencesPreferencesEditor::update_keys()
{
    for (auto &it : key_sequences_store->children()) {
        for (auto &it2 : it->children()) {
            if (in_tool_keyseq_preferences.keys.count(it2[tree_columns.action])) {
                std::stringstream s;
                const auto &seqs = in_tool_keyseq_preferences.keys.at(it2[tree_columns.action]);
                std::transform(seqs.begin(), seqs.end(), std::ostream_iterator<std::string>(s, ", "),
                               [](const auto &x) { return key_sequence_to_string(x); });
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

void InToolKeySequencesPreferencesEditor::handle_save()
{
    auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));
    std::string filename;
    {

        GtkFileChooserNative *native = gtk_file_chooser_native_new("Save keybindings", GTK_WINDOW(top->gobj()),
                                                                   GTK_FILE_CHOOSER_ACTION_SAVE, "_Save", "_Cancel");
        auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
        chooser->set_do_overwrite_confirmation(true);
        chooser->set_current_name("in-tool-keys.json");

        if (native_dialog_run_and_set_parent_insensitive(GTK_NATIVE_DIALOG(native)) == GTK_RESPONSE_ACCEPT) {
            filename = chooser->get_filename();
            if (!endswith(filename, ".json"))
                filename += ".json";
        }
    }
    if (filename.size()) {
        while (1) {
            std::string error_str;
            try {
                auto j = in_tool_keyseq_preferences.serialize();
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


void InToolKeySequencesPreferencesEditor::handle_load()
{
    auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));
    GtkFileChooserNative *native = gtk_file_chooser_native_new("Open keybindings", GTK_WINDOW(top->gobj()),
                                                               GTK_FILE_CHOOSER_ACTION_OPEN, "_Open", "_Cancel");
    auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
    auto filter = Gtk::FileFilter::create();
    filter->set_name("Horizon key bindings");
    filter->add_pattern("*.json");
    chooser->add_filter(filter);

    if (native_dialog_run_and_set_parent_insensitive(GTK_NATIVE_DIALOG(native)) == GTK_RESPONSE_ACCEPT) {
        auto filename = chooser->get_filename();
        std::string error_str;
        try {
            auto j = load_json_from_file(filename);
            in_tool_keyseq_preferences.load_from_json(j);
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

void InToolKeySequencesPreferencesEditor::handle_load_default()
{
    in_tool_keyseq_preferences.load_from_json(
            json_from_resource("/org/horizon-eda/horizon/imp/in_tool_keys_default.json"));
    update_action_editors();
    update_keys();
    preferences.signal_changed().emit();
}

InToolKeySequencesPreferencesEditor *InToolKeySequencesPreferencesEditor::create(Preferences &prefs)
{
    InToolKeySequencesPreferencesEditor *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/horizon-eda/horizon/pool-prj-mgr/preferences/preferences.ui", "key_sequences_box");
    x->get_widget_derived("key_sequences_box", w, prefs);
    w->reference();
    return w;
}
} // namespace horizon
