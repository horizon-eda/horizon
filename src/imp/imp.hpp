#pragma once
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
#include <nlohmann/json.hpp>
#include "search/searcher.hpp"
#include <zmq.hpp>
#include "util/item_set.hpp"
#include "canvas/canvas_gl.hpp"
#include "grid_controller.hpp"
#include "util/action_label.hpp"
#include <optional>
#include "core/clipboard/clipboard.hpp"
#include "clipboard_handler.hpp"
#include "util/win32_undef.hpp"
#include "logger/log_dispatcher.hpp"

namespace horizon {

class PoolParams {
public:
    PoolParams(const std::string &bp) : base_path(bp)
    {
    }
    std::string base_path;
};

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
    void set_suggested_filename(const std::string &s);
    enum class TempMode { YES, NO };

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
    LayerProvider &get_layer_provider() const;

protected:
    MainWindow *main_window;
    CanvasGL *canvas;
    class PropertyPanels *panels;
    WarningsBox *warnings_box;
    class ToolPopover *tool_popover;
    Gtk::Menu *context_menu = nullptr;
    std::unique_ptr<SelectionFilterDialog> selection_filter_dialog;
    std::optional<GridController> grid_controller;
    class GridsWindow *grids_window = nullptr;

    std::unique_ptr<Pool> pool;

private:
    std::unique_ptr<Pool> real_pool_caching;

protected:
    Pool *pool_caching;
    class Core *core = nullptr;
    std::unique_ptr<ClipboardBase> clipboard = nullptr;
    std::unique_ptr<ClipboardHandler> clipboard_handler = nullptr;
    std::unique_ptr<KeySequenceDialog> key_sequence_dialog = nullptr;
    std::unique_ptr<ImpInterface> imp_interface = nullptr;
    Glib::RefPtr<Glib::Binding> grid_spacing_binding;

    std::map<ActionToolID, ActionConnection> action_connections;

    ActionConnection &connect_action(ToolID tool_id);
    ActionConnection &connect_action(ActionToolID id, std::function<void(const ActionConnection &)> cb);
    ActionConnection &connect_action_with_source(ActionToolID id,
                                                 std::function<void(const ActionConnection &, ActionSource)> cb);

    class RulesWindow *rules_window = nullptr;

    zmq::context_t zctx;
    zmq::socket_t sock_broadcast_rx;
    zmq::socket_t sock_project;
    bool sockets_connected = false;
    int mgr_pid = -1;
    UUID ipc_cookie;
    bool no_update = false;
    bool distraction_free = false;

    virtual void canvas_update() = 0;
    virtual void expand_selection_for_property_panel(std::set<SelectableRef> &sel_extra,
                                                     const std::set<SelectableRef> &sel);
    void handle_selection_changed(void);
    virtual void handle_selection_cross_probe()
    {
    }
    bool handle_key_press(const GdkEventKey *key_event);
    void handle_cursor_move(const Coordi &pos);
    bool handle_click(const GdkEventButton *button_event);
    virtual void handle_extra_button(const GdkEventButton *button_event)
    {
    }
    bool handle_click_release(const GdkEventButton *button_event);
    bool handle_context_menu(const GdkEventButton *button_event);
    void tool_process(ToolResponse &resp);
    void tool_begin(ToolID id, bool override_selection = false, const std::set<SelectableRef> &sel = {},
                    std::unique_ptr<ToolData> data = nullptr);
    void add_tool_button(ToolID id, const std::string &label, bool left = true);
    void handle_warning_selected(const Coordi &pos);
    virtual bool handle_broadcast(const json &j);
    bool handle_close(const GdkEventAny *ev);
    json send_json(const json &j);

    bool trigger_action(ActionToolID action, ActionSource source = ActionSource::UNKNOWN);

    void connect_go_to_project_manager_action();

    void add_tool_action(ActionToolID id, const std::string &action);
    void add_hamburger_menu();

    Preferences preferences;

    virtual const CanvasPreferences &get_canvas_preferences()
    {
        return preferences.canvas_non_layer;
    }
    virtual void apply_preferences();

    std::unique_ptr<WindowStateStore> state_store = nullptr;

    virtual void handle_maybe_drag(bool ctrl);

    virtual ActionCatalogItem::Availability get_editor_type_for_action() const = 0;
    ObjectType get_editor_type() const;

    void layer_up_down(bool up);
    void goto_layer(int layer);

    Gtk::Button *create_action_button(ActionToolID action);
    void attach_action_button(Gtk::Button &button, ActionToolID action);

    void set_action_sensitive(ActionID, bool v);
    bool get_action_sensitive(ActionID) const;
    virtual void update_action_sensitivity();

    typedef sigc::signal<void> type_signal_action_sensitive;
    type_signal_action_sensitive signal_action_sensitive()
    {
        return s_signal_action_sensitive;
    }

    virtual std::string get_hud_text(std::set<SelectableRef> &sel);
    std::string get_hud_text_for_component(const Component *comp);
    virtual const Block &get_block_for_group_tag_names();
    std::string get_hud_text_for_net(const Net *net);
    std::string get_hud_text_for_layers(const std::set<int> &layers);

    void set_monitor_files(const std::set<std::string> &files);
    void set_monitor_items(const ItemSet &items);
    virtual void update_monitor()
    {
    }
    void edit_pool_item(ObjectType type, const UUID &uu);

    void parameter_window_add_polygon_expand(class ParameterWindow *parameter_window);

    bool read_only = false;

    void tool_update_data(std::unique_ptr<ToolData> data);

    virtual void search_center(const Searcher::SearchResult &res);
    virtual ActionToolID get_doubleclick_action(ObjectType type, const UUID &uu);

    Glib::RefPtr<Gio::Menu> hamburger_menu;

    json m_meta;
    void load_meta();
    static const std::string meta_suffix;

    virtual void get_save_meta(json &j)
    {
    }

    virtual void set_window_title(const std::string &s);
    void set_window_title_from_block();

    void update_view_hints();
    virtual std::vector<std::string> get_view_hints();

    Gtk::Box *view_options_menu = nullptr;
    void view_options_menu_append_action(const std::string &label, const std::string &action);

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
    bool force_end_tool();
    void reset_tool_hint_label();

    std::set<ObjectRef> highlights;

    virtual void update_highlights()
    {
    }
    virtual void clear_highlights();

    enum class ShowInPoolManagerPool { CURRENT, LAST };
    void show_in_pool_manager(ObjectType type, const UUID &uu, ShowInPoolManagerPool p);

    virtual bool uses_dynamic_version() const
    {
        return false;
    }

    virtual unsigned int get_required_version() const
    {
        throw std::runtime_error("not implemented");
    }

    bool temp_mode = false;
    std::string suggested_filename;

    void selection_load(const HistoryManager::HistoryItem &it);
    bool selection_loading = false;

    static void selection_push(HistoryManager &mgr, const std::set<SelectableRef> &selection);

    bool handle_pool_cache_update(const json &j);

private:
    void fix_cursor_pos();
    Glib::RefPtr<Gio::FileMonitor> preferences_monitor;
    void handle_drag();
    void update_selection_label();
    std::string get_tool_settings_filename(ToolID id);

    void hud_update();

    std::map<std::string, Glib::RefPtr<Gio::FileMonitor>> file_monitors;
    sigc::connection file_monitor_delay_connection;

    void handle_file_changed(const Glib::RefPtr<Gio::File> &file1, const Glib::RefPtr<Gio::File> &file2,
                             Gio::FileMonitorEvent ev);
    std::chrono::time_point<std::chrono::system_clock> last_pool_reload_time;

    void create_context_menu(Gtk::Menu *parent, const std::set<SelectableRef> &sel);
    Gtk::MenuItem *create_context_menu_item(ActionToolID act);

    KeySequence keys_current;
    KeyMatchResult keys_match(const KeySequence &keys) const;
    bool handle_action_key(const GdkEventKey *ev);
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

    LogDispatcher log_dispatcher;
    class LogWindow *log_window = nullptr;
    std::set<SelectableRef> selection_for_drag_move;
    ToolID drag_tool;
    Coordf cursor_pos_drag_begin;
    Coordi cursor_pos_grid_drag_begin;

    std::map<ActionID, bool> action_sensitivity;
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
    Glib::RefPtr<Gio::SimpleAction> show_pictures_action;

    int left_panel_width = 0;

    void tool_bar_set_actions(const std::vector<ActionLabelInfo> &labels);
    void tool_bar_append_action(InToolActionID action1, InToolActionID action2, const std::string &s);
    void tool_bar_clear_actions();
    InToolKeySequencesPreferences in_tool_key_sequeces_preferences;
    std::vector<ActionLabelInfo> in_tool_action_label_infos;

    void tool_process_one();

    void show_preferences(std::optional<std::string> page = std::nullopt);

    void init_search();
    void init_key();
    void init_action();

    void handle_pan_action(const ActionConnection &c);
    void handle_zoom_action(const ActionConnection &c);

    std::string get_complete_display_name(const SelectableRef &sr);

    void set_flip_view(bool flip);
    void apply_arrow_keys();
    void check_version();

    void update_cursor(ToolID tool_id);

    std::set<SelectableRef> last_canvas_selection;

    Gtk::Button *undo_button = nullptr;
    Gtk::Button *redo_button = nullptr;

    void selection_undo();
    void selection_redo();

    HistoryManager selection_history_manager;
    virtual HistoryManager &get_selection_history_manager();

    unsigned int saved_version = 0;

    class MSDTuningWindow *msd_tuning_window = nullptr;

    Gtk::Button *save_button = nullptr;
    virtual bool set_filename();

    std::set<std::string> modified_pool_items;
    sigc::connection pool_items_modified_connection;
    bool pool_reload_pending = false;
};
} // namespace horizon
