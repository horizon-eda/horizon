#pragma once
#include "common/common.hpp"
#include "rules/rules.hpp"
#include "rules/cache.hpp"
#include "util/uuid.hpp"
#include "util/changeable.hpp"
#include "util/window_state_store.hpp"
#include <array>
#include <gtkmm.h>
#include <set>
namespace horizon {

class RulesWindow : public Gtk::Window, public Changeable {
public:
    RulesWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, class CanvasGL *ca, class Rules *ru,
                class Core *c);
    static RulesWindow *create(Gtk::Window *p, class CanvasGL *ca, class Rules *ru, class Core *c);
    typedef sigc::signal<void, Coordi, UUID> type_signal_goto;
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

private:
    Gtk::ListBox *lb_rules = nullptr;
    Gtk::ListBox *lb_multi = nullptr;
    Gtk::Revealer *rev_multi = nullptr;
    Gtk::Button *button_rule_instance_add = nullptr;
    Gtk::Button *button_rule_instance_remove = nullptr;
    Gtk::Button *button_rule_instance_move_up = nullptr;
    Gtk::Button *button_rule_instance_move_down = nullptr;
    Gtk::Box *rule_editor_box = nullptr;
    Gtk::Button *run_button = nullptr;
    Gtk::Button *apply_button = nullptr;
    Gtk::Stack *stack = nullptr;
    Gtk::StackSwitcher *stack_switcher = nullptr;
    Gtk::Revealer *rev_warn = nullptr;
    Glib::RefPtr<Gtk::SizeGroup> sg_order;

    void rule_selected(RuleID id);
    void rule_instance_selected(RuleID id, const UUID &uu);
    void update_rule_instances(RuleID id);
    void update_rule_instance_labels();
    void update_rules_enabled();
    void update_warning();


    CanvasGL *canvas = nullptr;
    Rules *rules = nullptr;
    Core *core = nullptr;
    class CanvasAnnotation *annotation;

    class Block *get_block();
    type_signal_goto s_signal_goto;
    type_signal_canvas_update s_signal_canvas_update;
    RuleID rule_current = RuleID::NONE;

    class RuleEditor *editor = nullptr;
    void show_editor(RuleEditor *e);
    RuleEditor *create_editor(Rule *r);

    class TreeColumns : public Gtk::TreeModelColumnRecord {
    public:
        TreeColumns()
        {
            Gtk::TreeModelColumnRecord::add(name);
            Gtk::TreeModelColumnRecord::add(result);
            Gtk::TreeModelColumnRecord::add(has_location);
            Gtk::TreeModelColumnRecord::add(location);
            Gtk::TreeModelColumnRecord::add(sheet);
            Gtk::TreeModelColumnRecord::add(running);
            Gtk::TreeModelColumnRecord::add(status);
            Gtk::TreeModelColumnRecord::add(pulse);
        }
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<RulesCheckErrorLevel> result;
        Gtk::TreeModelColumn<bool> has_location;
        Gtk::TreeModelColumn<Coordi> location;
        Gtk::TreeModelColumn<UUID> sheet;
        Gtk::TreeModelColumn<bool> running;
        Gtk::TreeModelColumn<std::string> status;
        Gtk::TreeModelColumn<int> pulse;
    };
    TreeColumns tree_columns;

    Glib::RefPtr<Gtk::TreeStore> check_result_store;
    Gtk::TreeView *check_result_treeview = nullptr;

    Glib::Dispatcher dispatcher;

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

    std::unique_ptr<RulesCheckCache> cache;

    void check_thread(RuleID id);
    sigc::connection pulse_connection;

    WindowStateStore state_store;
    bool enabled = true;
};
} // namespace horizon
