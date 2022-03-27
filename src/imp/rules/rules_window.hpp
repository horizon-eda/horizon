#pragma once
#include "common/common.hpp"
#include "rules/rules.hpp"
#include "util/uuid.hpp"
#include "util/changeable.hpp"
#include "util/window_state_store.hpp"
#include <array>
#include <gtkmm.h>
#include <set>
#include <atomic>
namespace horizon {

class RulesWindow : public Gtk::Window, public Changeable {
public:
    RulesWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, class CanvasGL &ca, class Core &c,
                bool layered);
    static RulesWindow *create(Gtk::Window *p, class CanvasGL &ca, class Core &c, bool layered);
    typedef sigc::signal<void, Coordi, UUID, UUIDVec> type_signal_goto;
    type_signal_goto signal_goto()
    {
        return s_signal_goto;
    }

    typedef sigc::signal<void> type_signal_canvas_update;
    type_signal_canvas_update signal_canvas_update()
    {
        return s_signal_canvas_update;
    }

    void run_checks();
    void apply_rules();

    void set_enabled(bool enable);

    ~RulesWindow();

private:
    Gtk::ListBox *lb_rules = nullptr;
    Gtk::ListBox *lb_multi = nullptr;
    Gtk::Revealer *rev_multi = nullptr;
    Gtk::Button *button_rule_instance_add = nullptr;
    Gtk::Button *button_rule_instance_remove = nullptr;
    Gtk::Button *button_rule_instance_move_up = nullptr;
    Gtk::Button *button_rule_instance_move_down = nullptr;
    Gtk::Box *rule_editor_box = nullptr;
    Gtk::Box *run_box = nullptr;
    Gtk::Button *apply_button = nullptr;
    Gtk::Button *cancel_button = nullptr;
    Gtk::Stack *stack = nullptr;
    Gtk::StackSwitcher *stack_switcher = nullptr;
    Gtk::Revealer *rev_warn = nullptr;
    Glib::RefPtr<Gtk::SizeGroup> sg_order;
    Gtk::MenuItem *check_selected_rule_item = nullptr;

    void rule_selected(RuleID id);
    void rule_instance_selected(RuleID id, const UUID &uu);
    void update_rule_instances(RuleID id);
    void update_rule_instance_labels();
    void update_rule_labels();
    void update_warning();
    void cancel_checks();
    void update_can_check_selected();

    void run_check(RuleID rule);

    CanvasGL &canvas;
    Core &core;
    Rules &rules;
    const bool layered;
    class CanvasAnnotation *annotation = nullptr;

    class Block *get_top_block();
    type_signal_goto s_signal_goto;
    type_signal_canvas_update s_signal_canvas_update;
    RuleID rule_id_current = RuleID::NONE;

    class RuleEditor *editor = nullptr;
    void show_editor(RuleEditor *e);
    RuleEditor *create_editor(Rule &r);
    void reload_editor();

    enum class CheckState { NOT_RUNNING, RUNNING, CANCELLING };

    class TreeColumns : public Gtk::TreeModelColumnRecord {
    public:
        TreeColumns()
        {
            Gtk::TreeModelColumnRecord::add(rule);
            Gtk::TreeModelColumnRecord::add(name);
            Gtk::TreeModelColumnRecord::add(result);
            Gtk::TreeModelColumnRecord::add(has_location);
            Gtk::TreeModelColumnRecord::add(location);
            Gtk::TreeModelColumnRecord::add(sheet);
            Gtk::TreeModelColumnRecord::add(instance_path);
            Gtk::TreeModelColumnRecord::add(paths);
            Gtk::TreeModelColumnRecord::add(layers);
            Gtk::TreeModelColumnRecord::add(state);
            Gtk::TreeModelColumnRecord::add(status);
            Gtk::TreeModelColumnRecord::add(pulse);
            Gtk::TreeModelColumnRecord::add(outdated);
        }
        Gtk::TreeModelColumn<RuleID> rule;
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<RulesCheckErrorLevel> result;
        Gtk::TreeModelColumn<bool> has_location;
        Gtk::TreeModelColumn<Coordi> location;
        Gtk::TreeModelColumn<UUID> sheet;
        Gtk::TreeModelColumn<UUIDVec> instance_path;
        Gtk::TreeModelColumn<CheckState> state;
        Gtk::TreeModelColumn<ClipperLib::Paths> paths;
        Gtk::TreeModelColumn<std::set<int>> layers;
        Gtk::TreeModelColumn<std::string> status;
        Gtk::TreeModelColumn<int> pulse;
        Gtk::TreeModelColumn<bool> outdated;
    };
    TreeColumns tree_columns;

    Glib::RefPtr<Gtk::TreeStore> check_result_store;
    Glib::RefPtr<Gtk::TreeModelFilter> check_result_filter;
    Gtk::CheckButton *work_layer_only_checkbutton = nullptr;
    Gtk::Box *work_layer_only_box = nullptr;
    Gtk::TreeView *check_result_treeview = nullptr;

    class RuleRunInfo {
    public:
        RuleRunInfo(Gtk::TreeModel::Row &r) : row(r)
        {
        }
        RulesCheckResult result;
        std::string status;
        Gtk::TreeModel::Row row;
    };

    std::map<RuleID, RuleRunInfo> run_store;
    std::mutex run_store_mutex;

    std::unique_ptr<class RulesCheckCache> cache;

    void check_thread(RuleID id);

    WindowStateStore state_store;
    bool enabled = true;

    Glib::RefPtr<Gio::Menu> hamburger_menu;
    Gtk::MenuButton *hamburger_menu_button = nullptr;
    Glib::RefPtr<Gio::SimpleActionGroup> action_group;

    bool update_results();
    void update_markers_and_error_polygons();

    std::atomic_bool cancel_flag;
    bool queue_close = false;

    void export_rules();
    void import_rules();
};
} // namespace horizon
