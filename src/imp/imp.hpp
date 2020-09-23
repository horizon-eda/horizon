#pragma once
#include "core/clipboard.hpp"
#include "core/core.hpp"
#include "imp_interface.hpp"
#include "keyseq_dialog.hpp"
#include "main_window.hpp"
#include "pool/pool.hpp"
#include "preferences/preferences.hpp"
#include "selection_filter_dialog.hpp"
#include "util/window_state_store.hpp"
#include "widgets/spin_button_dim.hpp"
#include "widgets/warnings_box.hpp"
#include "action.hpp"
#include "nlohmann/json.hpp"
#include "search/searcher.hpp"
#include <zmq.hpp>
#include "util/item_set.hpp"
#include "canvas/canvas_gl.hpp"
#include "grid_controller.hpp"
#include "util/action_label.hpp"
#include <optional>

#ifdef G_OS_WIN32
#undef DELETE
#undef DUPLICATE
#undef ERROR
#endif

namespace horizon {

class PoolParams {
public:
    PoolParams(const std::string &bp, const std::string &cp = "") : base_path(bp), cache_path(cp)
    {
    }
    std::string base_path;
    std::string cache_path;
};

std::unique_ptr<Pool> make_pool(const PoolParams &params);

class ImpBase {
    friend class ImpInterface;

public:
    ImpBase(const PoolParams &params);
    void run(int argc, char *argv[]);
    virtual void handle_tool_change(ToolID id);
    virtual void construct() = 0;
    void canvas_update_from_pp();
    virtual ~ImpBase()
    {
    }
    void set_read_only(bool v);

    std::set<ObjectRef> highlights;
    virtual void update_highlights(){};

    class SelectionFilterInfo {
    public:
        enum class Flag {
            DEFAULT,
            HAS_OTHERS,
            WORK_LAYER_ONLY_ENABLED,
        };
        SelectionFilterInfo()
        {
        }
        SelectionFilterInfo(const std::vector<int> &l, Flag fl) : layers(l), flags(fl)
        {
        }
        std::vector<int> layers;
        Flag flags = Flag::DEFAULT;
    };

    virtual std::map<ObjectType, SelectionFilterInfo> get_selection_filter_info() const;
    virtual bool is_layered() const
    {
        return false;
    };

protected:
    MainWindow *main_window;
    CanvasGL *canvas;
    class PropertyPanels *panels;
    WarningsBox *warnings_box;
    class ToolPopover *tool_popover;
    Gtk::Menu *context_menu = nullptr;
    std::unique_ptr<SelectionFilterDialog> selection_filter_dialog;
    std::optional<GridController> grid_controller;

    std::unique_ptr<Pool> pool;
    class Core *core = nullptr;
    std::unique_ptr<ClipboardManager> clipboard = nullptr;
    std::unique_ptr<KeySequenceDialog> key_sequence_dialog = nullptr;
    std::unique_ptr<ImpInterface> imp_interface = nullptr;
    Glib::RefPtr<Glib::Binding> grid_spacing_binding;

    std::map<ActionToolID, ActionConnection> action_connections;
    ActionConnection &connect_action(ToolID tool_id, std::function<void(const ActionConnection &)> cb);
    ActionConnection &connect_action(ToolID tool_id);
    ActionConnection &connect_action(ActionID action_id, std::function<void(const ActionConnection &)> cb);

    class RulesWindow *rules_window = nullptr;

    zmq::context_t zctx;
    zmq::socket_t sock_broadcast_rx;
    zmq::socket_t sock_project;
    bool sockets_connected = false;
    int mgr_pid = -1;
    bool no_update = false;
    bool distraction_free = false;

    virtual void canvas_update() = 0;
    virtual void expand_selection_for_property_panel(std::set<SelectableRef> &sel_extra,
                                                     const std::set<SelectableRef> &sel);
    void handle_selection_changed(void);
    bool handle_key_press(GdkEventKey *key_event);
    void handle_cursor_move(const Coordi &pos);
    bool handle_click(GdkEventButton *button_event);
    bool handle_click_release(GdkEventButton *button_event);
    bool handle_context_menu(GdkEventButton *button_event);
    void tool_process(ToolResponse &resp);
    void tool_begin(ToolID id, bool override_selection = false, const std::set<SelectableRef> &sel = {},
                    std::unique_ptr<ToolData> data = nullptr);
    void add_tool_button(ToolID id, const std::string &label, bool left = true);
    void handle_warning_selected(const Coordi &pos);
    virtual bool handle_broadcast(const json &j);
    bool handle_close(GdkEventAny *ev);
    json send_json(const json &j);

    bool trigger_action(const ActionToolID &action);
    bool trigger_action(ActionID aid);
    bool trigger_action(ToolID tid);

    void add_tool_action(ToolID tid, const std::string &action);
    void add_tool_action(ActionID aid, const std::string &action);
    void add_hamburger_menu();

    Preferences preferences;

    virtual CanvasPreferences *get_canvas_preferences()
    {
        return &preferences.canvas_non_layer;
    }
    virtual void apply_preferences();

    std::unique_ptr<WindowStateStore> state_store = nullptr;

    virtual void handle_maybe_drag();

    virtual ActionCatalogItem::Availability get_editor_type_for_action() const = 0;
    ObjectType get_editor_type() const;

    void layer_up_down(bool up);
    void goto_layer(int layer);

    Gtk::Button *create_action_button(ActionToolID action);

    void set_action_sensitive(ActionToolID, bool v);
    bool get_action_sensitive(ActionToolID) const;
    virtual void update_action_sensitivity();

    typedef sigc::signal<void> type_signal_action_sensitive;
    type_signal_action_sensitive signal_action_sensitive()
    {
        return s_signal_action_sensitive;
    }

    virtual std::string get_hud_text(std::set<SelectableRef> &sel);
    std::string get_hud_text_for_component(const Component *comp);
    std::string get_hud_text_for_net(const Net *net);

    void set_monitor_files(const std::set<std::string> &files);
    void set_monitor_items(const ItemSet &items);
    virtual void update_monitor()
    {
    }
    void edit_pool_item(ObjectType type, const UUID &uu);

    void parameter_window_add_polygon_expand(class ParameterWindow *parameter_window);

    bool read_only = false;

    void tool_update_data(std::unique_ptr<ToolData> &data);

    virtual void search_center(const Searcher::SearchResult &res);
    virtual ActionToolID get_doubleclick_action(ObjectType type, const UUID &uu);

    Glib::RefPtr<Gio::Menu> hamburger_menu;

    json m_meta;
    void load_meta();
    static const std::string meta_suffix;

    virtual void get_save_meta(json &j)
    {
    }

    void set_window_title(const std::string &s);
    void set_window_title_from_block();

    void update_view_hints();
    virtual std::vector<std::string> get_view_hints();

    Glib::RefPtr<Gio::Menu> view_options_menu;

    virtual Searcher *get_searcher_ptr()
    {
        return nullptr;
    }

    bool has_searcher()
    {
        return get_searcher_ptr();
    }

    Searcher &get_searcher()
    {
        auto s = get_searcher_ptr();
        if (!s)
            throw std::runtime_error("not implemented");
        return *s;
    }

    class ActionButton &add_action_button(ActionToolID action);
    class ActionButtonMenu &add_action_button_menu(const char *icon_name);
    class ActionButton &add_action_button_polygon();
    class ActionButton &add_action_button_line();

    virtual ToolID get_tool_for_drag_move(bool ctrl, const std::set<SelectableRef> &sel) const;

private:
    void fix_cursor_pos();
    Glib::RefPtr<Gio::FileMonitor> preferences_monitor;
    void handle_drag(bool ctrl);
    void update_selection_label();
    std::string get_tool_settings_filename(ToolID id);

    void hud_update();

    std::map<std::string, Glib::RefPtr<Gio::FileMonitor>> file_monitors;

    void handle_file_changed(const Glib::RefPtr<Gio::File> &file1, const Glib::RefPtr<Gio::File> &file2,
                             Gio::FileMonitorEvent ev);

    ActionConnection &connect_action(ActionID action_id, ToolID tool_id,
                                     std::function<void(const ActionConnection &)> cb);

    void create_context_menu(Gtk::Menu *parent, const std::set<SelectableRef> &sel);
    Gtk::MenuItem *create_context_menu_item(ActionToolID act);

    KeySequence keys_current;
    enum class MatchResult { NONE, PREFIX, COMPLETE };
    MatchResult keys_match(const KeySequence &keys) const;
    bool handle_action_key(GdkEventKey *ev);
    void handle_tool_action(const ActionConnection &conn);
    void handle_select_polygon(const ActionConnection &a);

    void handle_search();
    void search_go(int dir);
    std::list<Searcher::SearchResult> search_results;
    unsigned int search_result_current = 0;
    void update_search_markers();
    void update_search_types_label();
    void set_search_mode(bool enabled, bool focus = true);
    std::map<Searcher::Type, Gtk::CheckButton *> search_check_buttons;

    class LogWindow *log_window = nullptr;
    std::set<SelectableRef> selection_for_drag_move;
    Coordf cursor_pos_drag_begin;
    Coordi cursor_pos_grid_drag_begin;

    std::map<ActionToolID, bool> action_sensitivity;
    type_signal_action_sensitive s_signal_action_sensitive;

    bool property_panel_has_focus();

    sigc::connection initial_view_all_conn;

    bool sockets_broken = false;
    void show_sockets_broken_dialog(const std::string &msg = "");
    bool needs_autosave = false;
    bool queue_autosave = false;

    void update_property_panels();
    std::map<CanvasGL::SelectionTool, CanvasGL::SelectionQualifier> selection_qualifiers;
    std::list<class ActionButtonBase *> action_buttons;

    Glib::RefPtr<Gio::SimpleAction> bottom_view_action;
    Glib::RefPtr<Gio::SimpleAction> distraction_free_action;

    int left_panel_width = 0;

    void tool_bar_set_actions(const std::vector<ActionLabelInfo> &labels);
    void tool_bar_append_action(InToolActionID action1, InToolActionID action2, const std::string &s);
    void tool_bar_clear_actions();
    InToolKeySequencesPreferences in_tool_key_sequeces_preferences;
    std::vector<ActionLabelInfo> in_tool_action_label_infos;

    void show_preferences(std::optional<std::string> page = std::nullopt);

    void init_search();
    void init_key();
    void init_action();

    void handle_pan_action(const ActionConnection &c);
    void handle_zoom_action(const ActionConnection &c);

    void force_end_tool();

    std::string get_complete_display_name(const SelectableRef &sr);

    void set_flip_view(bool flip);
    void apply_arrow_keys();
};
} // namespace horizon
