#include "imp.hpp"
#include "core/tool_id.hpp"
#include "logger/logger.hpp"
#include "in_tool_action_catalog.hpp"
#include "actions.hpp"
#include "util/str_util.hpp"

namespace horizon {


class KeyLabel : public Gtk::Box {
public:
    KeyLabel(Glib::RefPtr<Gtk::SizeGroup> sg, const std::string &key_markup, ActionToolID act)
        : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8), action(act)
    {
        {
            auto la = Gtk::manage(new Gtk::Label);
            la->set_xalign(0);
            la->set_markup(key_markup);
            la->show();
            pack_start(*la, false, false, 0);
            sg->add_widget(*la);
        }
        {
            auto la = Gtk::manage(new Gtk::Label);
            la->set_xalign(0);
            la->set_text(action_catalog.at(action).name);
            la->show();
            pack_start(*la, true, true, 0);
        }
        property_margin() = 2;
    }

    const ActionToolID action;
};

void ImpBase::init_key()
{
    canvas->signal_key_press_event().connect(sigc::mem_fun(*this, &ImpBase::handle_key_press));
    key_sequence_dialog = std::make_unique<KeySequenceDialog>(this->main_window);
    connect_action(ActionID::HELP, [this](const auto &a) { key_sequence_dialog->show(); });
    main_window->key_hint_box->signal_row_activated().connect([this](auto row) {
        if (auto la = dynamic_cast<KeyLabel *>(row->get_child())) {
            trigger_action(la->action);
            keys_current.clear();
            main_window->key_hint_set_visible(false);
        }
    });
}

bool ImpBase::handle_key_press(const GdkEventKey *key_event)
{
    return handle_action_key(key_event);
    return false;
}

KeyMatchResult ImpBase::keys_match(const KeySequence &keys) const
{
    return key_sequence_match(keys_current, keys);
}

class KeyConflictDialog : public Gtk::MessageDialog {
public:
    KeyConflictDialog(Gtk::Window *parent, const std::list<std::pair<std::string, KeySequence>> &conflicts);
    enum Response { RESPONSE_PREFS = 1 };

private:
    class ListColumns : public Gtk::TreeModelColumnRecord {
    public:
        ListColumns()
        {
            Gtk::TreeModelColumnRecord::add(name);
            Gtk::TreeModelColumnRecord::add(keys);
        }
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<Glib::ustring> keys;
    };
    ListColumns list_columns;

    Glib::RefPtr<Gtk::ListStore> store;

    Gtk::TreeView *tv = nullptr;
};


KeyConflictDialog::KeyConflictDialog(Gtk::Window *parent,
                                     const std::list<std::pair<std::string, KeySequence>> &conflicts)
    : Gtk::MessageDialog(*parent, "Key sequences conflict", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK)
{
    set_secondary_text("The key sequences listed below conflict with each other");
    add_button("Open preferences", RESPONSE_PREFS);

    store = Gtk::ListStore::create(list_columns);

    tv = Gtk::manage(new Gtk::TreeView(store));
    tv->get_selection()->set_mode(Gtk::SELECTION_NONE);
    tv->append_column("Action", list_columns.name);
    tv->append_column("Key sequence", list_columns.keys);


    auto sc = Gtk::manage(new Gtk::ScrolledWindow);
    sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    sc->set_shadow_type(Gtk::SHADOW_IN);
    sc->property_margin() = 10;
    sc->set_propagate_natural_height(true);
    sc->add(*tv);

    for (const auto &[act, keys] : conflicts) {
        Gtk::TreeModel::Row row = *store->append();
        row[list_columns.name] = act;
        row[list_columns.keys] = key_sequence_to_string(keys);
    }

    get_content_area()->pack_start(*sc, true, true, 0);
    get_content_area()->show_all();
    get_content_area()->set_spacing(0);
}

bool ImpBase::handle_action_key(const GdkEventKey *ev)
{
    if (ev->is_modifier)
        return false;
    if (ev->keyval == GDK_KEY_Escape) {
        if (!core->tool_is_active()) {
            canvas->set_selection_mode(CanvasGL::SelectionMode::HOVER);
            canvas->set_selection({});
            set_search_mode(false);

            reset_tool_hint_label();
            if (keys_current.size() == 0) {
                return false;
            }
            else {
                main_window->key_hint_set_visible(false);
                keys_current.clear();
                return true;
            }
        }
        else {
            ToolArgs args;
            args.coords = canvas->get_cursor_pos();
            args.work_layer = canvas->property_work_layer();
            args.type = ToolEventType::ACTION;
            args.action = InToolActionID::CANCEL;
            ToolResponse r = core->tool_update(args);
            tool_process(r);
            return true;
        }
    }
    else {
        auto display = main_window->get_display()->gobj();
        auto hw_keycode = ev->hardware_keycode;
        auto state = static_cast<GdkModifierType>(ev->state);
        auto group = ev->group;
        guint keyval;
        GdkModifierType consumed_modifiers;
        if (gdk_keymap_translate_keyboard_state(gdk_keymap_get_for_display(display), hw_keycode, state, group, &keyval,
                                                NULL, NULL, &consumed_modifiers)) {
            auto mod = static_cast<GdkModifierType>((state & (~consumed_modifiers))
                                                    & (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK));
            keys_current.emplace_back(keyval, mod);
        }
        auto in_tool_actions = core->get_tool_actions();
        std::map<InToolActionID, std::pair<KeyMatchResult, KeySequence>> in_tool_actions_matched;
        std::map<ActionConnection *, std::pair<KeyMatchResult, KeySequence>> connections_matched;
        auto selection = canvas->get_selection();
        update_action_sensitivity();
        for (auto &it : action_connections) {
            const auto k = it.second.id;
            if (action_catalog.at(k).availability & get_editor_type_for_action()) {
                bool can_begin = false;
                if (it.second.id.is_tool() && !core->tool_is_active()) {
                    can_begin = core->tool_can_begin(it.second.id.tool, selection).first;
                }
                else if (it.second.id.is_action()) {
                    can_begin = get_action_sensitive(k.action);
                }
                if (can_begin) {
                    for (const auto &it2 : it.second.key_sequences) {
                        if (const auto m = keys_match(it2); m != KeyMatchResult::NONE) {
                            connections_matched.emplace(std::piecewise_construct, std::forward_as_tuple(&it.second),
                                                        std::forward_as_tuple(m, it2));
                        }
                    }
                }
            }
        }
        if (in_tool_actions.size()) {
            for (const auto &[action, seqs] : in_tool_key_sequeces_preferences.keys) {
                if (in_tool_actions.count(action)) {
                    for (const auto &seq : seqs) {
                        if (const auto m = keys_match(seq); m != KeyMatchResult::NONE)
                            in_tool_actions_matched.emplace(std::piecewise_construct, std::forward_as_tuple(action),
                                                            std::forward_as_tuple(m, seq));
                    }
                }
            }
        }

        if (connections_matched.size() == 1 && in_tool_actions_matched.size() == 1) {
            main_window->tool_hint_label->set_text("Ambiguous");
            keys_current.clear();
        }
        else if (connections_matched.size() == 1) {
            main_window->tool_hint_label->set_text(key_sequence_to_string(keys_current));
            keys_current.clear();
            main_window->key_hint_set_visible(false);
            auto conn = connections_matched.begin()->first;
            if (!trigger_action(conn->id, ActionSource::KEY)) {
                reset_tool_hint_label();
                return false;
            }
            return true;
        }
        else if (in_tool_actions_matched.size() == 1) {
            main_window->tool_hint_label->set_text(key_sequence_to_string(keys_current));
            keys_current.clear();

            ToolArgs args;
            args.coords = canvas->get_cursor_pos();
            args.work_layer = canvas->property_work_layer();
            args.type = ToolEventType::ACTION;
            args.action = in_tool_actions_matched.begin()->first;
            ToolResponse r = core->tool_update(args);
            tool_process(r);

            return true;
        }
        else if (connections_matched.size() > 1 || in_tool_actions_matched.size() > 1) { // still ambigous
            std::list<std::pair<std::string, KeySequence>> conflicts;
            bool have_conflict = false;
            for (const auto &[conn, it] : connections_matched) {
                const auto &[res, seq] = it;
                if (res == KeyMatchResult::COMPLETE) {
                    have_conflict = true;
                }
                conflicts.emplace_back(action_catalog.at(conn->id).name, seq);
            }
            for (const auto &[act, it] : in_tool_actions_matched) {
                const auto &[res, seq] = it;
                if (res == KeyMatchResult::COMPLETE) {
                    have_conflict = true;
                }
                conflicts.emplace_back(in_tool_action_catalog.at(act).name + " (in-tool action)", seq);
            }
            if (have_conflict) {
                main_window->tool_hint_label->set_text("Key sequences conflict");
                keys_current.clear();
                KeyConflictDialog dia(main_window, conflicts);
                if (dia.run() == KeyConflictDialog::RESPONSE_PREFS) {
                    show_preferences("keys");
                }
                return false;
            }

            for (auto ch : main_window->key_hint_box->get_children()) {
                delete ch;
            }

            for (const auto &[conn, it] : connections_matched) {
                const auto &[res, seq] = it;
                std::string seq_label;
                for (size_t i = 0; i < seq.size(); i++) {
                    seq_label += Glib::Markup::escape_text(key_sequence_item_to_string(seq.at(i))) + " ";
                    if (i + 1 == keys_current.size()) {
                        seq_label += "<b>";
                    }
                }
                rtrim(seq_label);
                seq_label += "</b>";

                auto la = Gtk::manage(new KeyLabel(main_window->key_hint_size_group, seq_label, conn->id));
                main_window->key_hint_box->append(*la);
                la->show();
            }
            main_window->key_hint_set_visible(true);

            main_window->tool_hint_label->set_text(key_sequence_to_string(keys_current) + "?");
            return true;
        }
        else if (connections_matched.size() == 0 || in_tool_actions_matched.size() == 0) {
            main_window->tool_hint_label->set_text("Unknown key sequence");
            keys_current.clear();
            main_window->key_hint_set_visible(false);
            return false;
        }
        else {
            Logger::log_warning("Key sequence??", Logger::Domain::IMP,
                                std::to_string(connections_matched.size()) + " "
                                        + std::to_string(in_tool_actions_matched.size()));
            return false;
        }
    }
    return false;
}

void ImpBase::apply_arrow_keys()
{
    const auto &canvas_prefs = get_canvas_preferences();

    const auto left = GDK_KEY_Left;
    const auto right = GDK_KEY_Right;
    const auto up = GDK_KEY_Up;
    const auto down = GDK_KEY_Down;

    {
        const auto mod0 = static_cast<GdkModifierType>(0);
        action_connections.at(ToolID::MOVE_KEY_LEFT).key_sequences = {{{left, mod0}}};
        action_connections.at(ToolID::MOVE_KEY_RIGHT).key_sequences = {{{right, mod0}}};
        action_connections.at(ToolID::MOVE_KEY_UP).key_sequences = {{{up, mod0}}};
        action_connections.at(ToolID::MOVE_KEY_DOWN).key_sequences = {{{down, mod0}}};

        in_tool_key_sequeces_preferences.keys[InToolActionID::MOVE_LEFT] = {{{left, mod0}}};
        in_tool_key_sequeces_preferences.keys[InToolActionID::MOVE_RIGHT] = {{{right, mod0}}};
        in_tool_key_sequeces_preferences.keys[InToolActionID::MOVE_UP] = {{{up, mod0}}};
        in_tool_key_sequeces_preferences.keys[InToolActionID::MOVE_DOWN] = {{{down, mod0}}};
    }
    {
        GdkModifierType grid_fine_modifier = GDK_MOD1_MASK;

        switch (canvas_prefs.appearance.grid_fine_modifier) {
        case Appearance::GridFineModifier::ALT:
            grid_fine_modifier = GDK_MOD1_MASK;
            break;
        case Appearance::GridFineModifier::CTRL:
            grid_fine_modifier = GDK_CONTROL_MASK;
            break;
        }
        action_connections.at(ToolID::MOVE_KEY_FINE_LEFT).key_sequences = {{{left, grid_fine_modifier}}};
        action_connections.at(ToolID::MOVE_KEY_FINE_RIGHT).key_sequences = {{{right, grid_fine_modifier}}};
        action_connections.at(ToolID::MOVE_KEY_FINE_UP).key_sequences = {{{up, grid_fine_modifier}}};
        action_connections.at(ToolID::MOVE_KEY_FINE_DOWN).key_sequences = {{{down, grid_fine_modifier}}};
        in_tool_key_sequeces_preferences.keys[InToolActionID::MOVE_LEFT_FINE] = {{{left, grid_fine_modifier}}};
        in_tool_key_sequeces_preferences.keys[InToolActionID::MOVE_RIGHT_FINE] = {{{right, grid_fine_modifier}}};
        in_tool_key_sequeces_preferences.keys[InToolActionID::MOVE_UP_FINE] = {{{up, grid_fine_modifier}}};
        in_tool_key_sequeces_preferences.keys[InToolActionID::MOVE_DOWN_FINE] = {{{down, grid_fine_modifier}}};
    }
}

} // namespace horizon
