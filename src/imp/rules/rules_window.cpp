#include <widgets/cell_renderer_color_box.hpp>
#include "rules_window.hpp"
#include "canvas/canvas_gl.hpp"
#include "rule_editor.hpp"
#include "rule_editor_clearance_copper.hpp"
#include "rule_editor_clearance_copper_other.hpp"
#include "rule_editor_clearance_silk_exp_copper.hpp"
#include "rule_editor_diffpair.hpp"
#include "rule_editor_hole_size.hpp"
#include "rule_editor_package_checks.hpp"
#include "rule_editor_plane.hpp"
#include "rule_editor_single_pin_net.hpp"
#include "rule_editor_track_width.hpp"
#include "rule_editor_via.hpp"
#include "rule_editor_clearance_copper_keepout.hpp"
#include "rule_editor_layer_pair.hpp"
#include "rule_editor_clearance_same_net.hpp"
#include "rule_editor_shorted_pads.hpp"
#include "rules/cache.hpp"
#include "rules/rule_descr.hpp"
#include "rules/rules.hpp"
#include "rules/rules_with_core.hpp"
#include "util/util.hpp"
#include "core/core.hpp"
#include "util/gtk_util.hpp"
#include "nlohmann/json.hpp"
#include "import.hpp"
#include "export.hpp"

#include <thread>

namespace horizon {

class RuleLabel : public Gtk::Box {
public:
    RuleLabel(Glib::RefPtr<Gtk::SizeGroup> sg, const std::string &t, RuleID i, const UUID &uu = UUID(), int o = -1)
        : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4), id(i), uuid(uu), order(o)
    {
        set_margin_top(5);
        set_margin_bottom(5);
        set_margin_left(5);
        set_margin_right(5);

        label_order = Gtk::manage(new Gtk::Label(std::to_string(o)));
        label_order->set_xalign(1);
        label_text = Gtk::manage(new Gtk::Label());
        label_text->set_markup(t);
        label_text->set_xalign(0);
        pack_start(*label_order, false, false, 0);
        pack_start(*label_text, true, true, 0);
        if (o >= 0) {
            label_order->show();
            sg->add_widget(*label_order);
        }
        label_text->show();

        img_imported = Gtk::manage(new Gtk::Image);
        img_imported->set_margin_start(4);
        img_imported->set_from_icon_name("rule-imported-symbolic", Gtk::ICON_SIZE_BUTTON);
        img_imported->set_tooltip_text("This rule has recently been imported");
        pack_start(*img_imported, false, false, 0);
    }

    void set_text(const std::string &t)
    {
        label_text->set_markup(t);
    }

    void set_imported(bool imported)
    {
        img_imported->set_visible(imported);
    }

    void set_order(int o)
    {
        order = o;
        label_order->set_text(std::to_string(order));
    }
    int get_order() const
    {
        return order;
    }
    const RuleID id;
    const UUID uuid;

private:
    int order = 0;
    Gtk::Label *label_order = nullptr;
    Gtk::Label *label_text = nullptr;
    Gtk::Image *img_imported = nullptr;
};

RulesWindow::RulesWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, CanvasGL &ca, class Core &c)
    : Gtk::Window(cobject), canvas(ca), core(c), rules(*core.get_rules()),
      state_store(this, "imp-rules-" + object_type_lut.lookup_reverse(core.get_object_type()))
{
    GET_WIDGET(lb_rules);
    GET_WIDGET(lb_multi);
    GET_WIDGET(rev_multi);
    GET_WIDGET(button_rule_instance_add);
    GET_WIDGET(button_rule_instance_remove);
    GET_WIDGET(button_rule_instance_move_up);
    GET_WIDGET(button_rule_instance_move_down);
    GET_WIDGET(rule_editor_box);
    GET_WIDGET(check_result_treeview);
    GET_WIDGET(run_button);
    GET_WIDGET(apply_button);
    GET_WIDGET(stack);
    GET_WIDGET(stack_switcher);
    GET_WIDGET(rev_warn);
    sg_order = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);

    lb_rules->signal_row_selected().connect(
            [this](Gtk::ListBoxRow *row) { rule_selected(dynamic_cast<RuleLabel *>(row->get_child())->id); });
    for (const auto id : rules.get_rule_ids()) {
        const auto &desc = rule_descriptions.at(id);
        auto la = Gtk::manage(new RuleLabel(sg_order, desc.name, id));
        la->show();
        lb_rules->append(*la);
    }

    {
        const auto ids = rules.get_rule_ids();
        const bool can_apply =
                std::any_of(ids.begin(), ids.end(), [](const auto id) { return rule_descriptions.at(id).can_apply; });
        if (!can_apply)
            apply_button->hide();
    }

    lb_multi->signal_selected_rows_changed().connect([this] {
        bool selected = lb_multi->get_selected_row() != nullptr;
        button_rule_instance_remove->set_sensitive(selected && enabled);
        button_rule_instance_move_up->set_sensitive(selected && enabled);
        button_rule_instance_move_down->set_sensitive(selected && enabled);
    });
    lb_multi->signal_row_selected().connect([this](Gtk::ListBoxRow *row) {
        if (row) {
            auto la = dynamic_cast<RuleLabel *>(row->get_child());
            rule_instance_selected(la->id, la->uuid);
        }
        else {
            show_editor(nullptr);
        }
    });
    lb_multi->set_sort_func([](Gtk::ListBoxRow *a, Gtk::ListBoxRow *b) {
        auto la = dynamic_cast<RuleLabel *>(a->get_child());
        auto lb = dynamic_cast<RuleLabel *>(b->get_child());
        return la->get_order() - lb->get_order();
    });

    button_rule_instance_remove->signal_clicked().connect([this] {
        auto row = lb_multi->get_selected_row();
        if (row) {
            auto la = dynamic_cast<RuleLabel *>(row->get_child());
            rules.remove_rule(la->id, la->uuid);
            update_rule_instances(la->id);
            update_warning();
            s_signal_changed.emit();
        }
    });

    button_rule_instance_add->signal_clicked().connect([this] {
        rules.add_rule(rule_id_current);
        update_rule_instances(rule_id_current);
        lb_multi->select_row(*lb_multi->get_row_at_index(0));
        update_warning();
        s_signal_changed.emit();
    });

    button_rule_instance_move_up->signal_clicked().connect([this] {
        auto row = lb_multi->get_selected_row();
        if (row) {
            auto la = dynamic_cast<RuleLabel *>(row->get_child());
            rules.move_rule(la->id, la->uuid, -1);
            update_rule_instance_labels();
            update_warning();
            s_signal_changed.emit();
        }
    });
    button_rule_instance_move_down->signal_clicked().connect([this] {
        auto row = lb_multi->get_selected_row();
        if (row) {
            auto la = dynamic_cast<RuleLabel *>(row->get_child());
            rules.move_rule(la->id, la->uuid, 1);
            update_rule_instance_labels();
            update_warning();
            s_signal_changed.emit();
        }
    });
    run_button->signal_clicked().connect(sigc::mem_fun(*this, &RulesWindow::run_checks));
    apply_button->signal_clicked().connect(sigc::mem_fun(*this, &RulesWindow::apply_rules));
    update_rule_labels();

    check_result_store = Gtk::TreeStore::create(tree_columns);
    check_result_treeview->set_model(check_result_store);

    check_result_treeview->append_column("Rule", tree_columns.name);
    {
        auto cr = Gtk::manage(new CellRendererColorBox());
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Result"));
        tvc->pack_start(*cr, false);
        auto cr2 = Gtk::manage(new Gtk::CellRendererText());
        tvc->pack_start(*cr2, false);
        auto cr3 = Gtk::manage(new Gtk::CellRendererSpinner());
        tvc->pack_start(*cr3, false);
        tvc->add_attribute(cr3->property_pulse(), tree_columns.pulse);
        cr3->property_xalign() = 0;
        auto cr4 = Gtk::manage(new Gtk::CellRendererText());
        tvc->pack_start(*cr4, false);

        tvc->set_cell_data_func(*cr2, [this](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
            Gtk::TreeModel::Row row = *it;
            auto mcr = dynamic_cast<Gtk::CellRendererText *>(tcr);
            if (row[tree_columns.running]) {
                mcr->property_text() = "Running";
            }
            else {
                mcr->property_text() = rules_check_error_level_to_string(row[tree_columns.result]);
            }
        });
        tvc->set_cell_data_func(*cr4, [this](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
            Gtk::TreeModel::Row row = *it;
            auto mcr = dynamic_cast<Gtk::CellRendererText *>(tcr);
            auto attributes_list = pango_attr_list_new();
            auto attribute_font_features = pango_attr_font_features_new("tnum 1");
            pango_attr_list_insert(attributes_list, attribute_font_features);
            g_object_set(G_OBJECT(mcr->gobj()), "attributes", attributes_list, NULL);
            pango_attr_list_unref(attributes_list);
            if (row[tree_columns.running]) {
                mcr->property_text() = static_cast<std::string>(row[tree_columns.status]);
            }
            else {
                mcr->property_text() = "";
            }
        });

        tvc->set_cell_data_func(*cr, [this](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
            Gtk::TreeModel::Row row = *it;
            auto mcr = dynamic_cast<CellRendererColorBox *>(tcr);
            auto co = rules_check_error_level_to_color(row[tree_columns.result]);
            Gdk::RGBA va;
            va.set_alpha(1);
            va.set_red(co.r);
            va.set_green(co.g);
            va.set_blue(co.b);
            mcr->property_color() = va;
        });
        tvc->set_cell_data_func(*cr3, [this](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
            Gtk::TreeModel::Row row = *it;
            auto mcr = dynamic_cast<Gtk::CellRendererSpinner *>(tcr);
            mcr->property_active() = row[tree_columns.running];
        });

        check_result_treeview->append_column(*tvc);
    }

    annotation = canvas.create_annotation();

    signal_show().connect([this] {
        canvas.markers.set_domain_visible(MarkerDomain::CHECK, true);
        annotation->set_visible(true);
    });
    signal_hide().connect([this] {
        canvas.markers.set_domain_visible(MarkerDomain::CHECK, false);
        annotation->set_visible(false);
    });

    check_result_treeview->signal_row_activated().connect(
            [this](const Gtk::TreeModel::Path &path, Gtk::TreeViewColumn *column) {
                auto it = check_result_store->get_iter(path);
                if (it) {
                    Gtk::TreeModel::Row row = *it;
                    if (row[tree_columns.has_location]) {
                        s_signal_goto.emit(row[tree_columns.location], row[tree_columns.sheet],
                                           row[tree_columns.instance_path]);
                    }
                }
            });

    dispatcher.connect([this] {
        std::lock_guard<std::mutex> guard(run_store_mutex);
        auto &dom = canvas.markers.get_domain(MarkerDomain::CHECK);
        for (const auto &it : run_store) {
            if (it.second.result.level != RulesCheckErrorLevel::NOT_RUN) { // has completed
                it.second.row[tree_columns.result] = it.second.result.level;
                it.second.row[tree_columns.running] = false;
                for (const auto &it_err : it.second.result.errors) {
                    auto iter = check_result_store->append(it.second.row.children());
                    Gtk::TreeModel::Row row_err = *iter;
                    row_err[tree_columns.result] = it_err.level;
                    row_err[tree_columns.name] = it_err.comment;
                    row_err[tree_columns.has_location] = it_err.has_location;
                    row_err[tree_columns.location] = it_err.location;
                    row_err[tree_columns.sheet] = it_err.sheet;
                    row_err[tree_columns.instance_path] = it_err.instance_path;
                    row_err[tree_columns.running] = false;

                    if (it_err.has_location) {
                        UUIDVec sheet;
                        if (it_err.sheet)
                            sheet = uuid_vec_append(it_err.instance_path, it_err.sheet);
                        dom.emplace_back(it_err.location, rules_check_error_level_to_color(it_err.level), sheet);
                    }

                    for (const auto &path : it_err.error_polygons) {
                        ClipperLib::IntPoint last = path.back();
                        for (const auto &pt : path) {
                            annotation->draw_line(Coordf(last.X, last.Y), Coordf(pt.X, pt.Y), ColorP::FROM_LAYER,
                                                  .01_mm);
                            last = pt;
                        }
                    }
                }
            }
            else { // still running
                it.second.row[tree_columns.status] = it.second.status;
            }
        }
        canvas.update_markers();
        check_result_treeview->expand_all();
        map_erase_if(run_store, [](auto &a) { return a.second.result.level != RulesCheckErrorLevel::NOT_RUN; });
        if (run_store.size() == 0) {
            run_button->set_sensitive(true);
            apply_button->set_sensitive(true);
            set_modal(false);
            pulse_connection.disconnect();
            stack_switcher->set_sensitive(true);
            dynamic_cast<Gtk::HeaderBar *>(get_titlebar())->set_show_close_button(true);
        }
    });

    action_group = Gio::SimpleActionGroup::create();
    insert_action_group("rules", action_group);
    action_group->add_action("export", sigc::mem_fun(*this, &RulesWindow::export_rules));
    action_group->add_action("import", sigc::mem_fun(*this, &RulesWindow::import_rules));

    Gtk::MenuButton *hamburger_menu_button;
    GET_WIDGET(hamburger_menu_button);
    if (rules.can_export()) {
        hamburger_menu = Gio::Menu::create();
        hamburger_menu_button->set_menu_model(hamburger_menu);
        hamburger_menu->append("Export…", "rules.export");
        hamburger_menu->append("Import…", "rules.import");
    }
    else {
        hamburger_menu_button->hide();
    }

    signal_delete_event().connect([this](GdkEventAny *ev) { return !run_button->get_sensitive(); });
}

void RulesWindow::apply_rules()
{
    if (core.tool_is_active())
        return;
    for (auto &rule : rules.get_rule_ids()) {
        rules_apply(rules, rule, core);
    }
    core.rebuild();
    s_signal_canvas_update.emit();
    s_signal_changed.emit();
}

void RulesWindow::export_rules()
{
    auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));
    RuleExportDialog dia;
    dia.set_transient_for(*top);
    if (dia.run() == Gtk::RESPONSE_OK) {
        std::string error_str;
        try {
            auto j = rules_export(rules, dia.get_export_info(), core);
            save_json_to_file(dia.get_filename(), j);
        }
        catch (const std::exception &e) {
            error_str = e.what();
        }
        catch (...) {
            error_str = "unknown error";
        }
        if (error_str.size()) {
            Gtk::MessageDialog edia(dia, "Export error", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
            edia.set_secondary_text(error_str);
            edia.run();
        }
    }
}

void RulesWindow::import_rules()
{
    auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));
    std::string filename;
    {
        GtkFileChooserNative *native = gtk_file_chooser_native_new("Import rules", GTK_WINDOW(top->gobj()),
                                                                   GTK_FILE_CHOOSER_ACTION_OPEN, "_Open", "_Cancel");
        auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
        auto filter = Gtk::FileFilter::create();
        filter->set_name("Horizon rules");
        filter->add_pattern("*.json");
        chooser->add_filter(filter);

        if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(native)) == GTK_RESPONSE_ACCEPT) {
            filename = chooser->get_filename();
        }
        else {
            return;
        }
    }
    std::string error_str;
    try {
        auto j = load_json_from_file(filename);
        auto import_info = rules_get_import_info(j);
        auto dia = make_rule_import_dialog(*import_info, core);
        dia->set_transient_for(*top);
        if (dia->run() == Gtk::RESPONSE_OK) {
            auto map = dia->get_import_map();
            rules.import_rules(import_info->get_rules(), *map);
            update_rule_labels();
            update_rule_instance_labels();
            reload_editor();
            s_signal_changed.emit();
        }
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

void RulesWindow::check_thread(RuleID id)
{
    RulesCheckResult result;
    try {
        result = rules_check(rules, id, core, *cache.get(), [this, id](const std::string &status) {
            {
                std::lock_guard<std::mutex> guard(run_store_mutex);
                run_store.at(id).status = status;
            }
            dispatcher.emit();
        });
    }

    catch (const std::exception &ex) {
        result.errors.emplace_back(RulesCheckErrorLevel::FAIL);
        auto &e = result.errors.back();
        e.comment = std::string("exception thrown: ") + ex.what();
        e.level = RulesCheckErrorLevel::FAIL;
        result.update();
    }
    catch (...) {
        result.errors.emplace_back(RulesCheckErrorLevel::FAIL);
        auto &e = result.errors.back();
        e.comment = "unknown exception thrown";
        e.level = RulesCheckErrorLevel::FAIL;
        result.update();
    }
    {
        {
            std::lock_guard<std::mutex> guard(run_store_mutex);
            run_store.at(id).result = result;
        }
        dispatcher.emit();
    }
}

void RulesWindow::run_checks()
{
    check_result_store->clear();
    auto &dom = canvas.markers.get_domain(MarkerDomain::CHECK);
    dom.clear();
    cache.reset(new RulesCheckCache(core));
    annotation->clear();
    run_store.clear();
    pulse_connection = Glib::signal_timeout().connect(sigc::track_obj(
                                                              [this] {
                                                                  for (auto &ch : check_result_store->children()) {
                                                                      ch[tree_columns.pulse] =
                                                                              ch[tree_columns.pulse] + 1;
                                                                  }
                                                                  return true;
                                                              },
                                                              *this),
                                                      750 / 12);
    for (auto rule_id : rules.get_rule_ids()) {
        if (rule_descriptions.at(rule_id).can_check) {
            Gtk::TreeModel::iterator iter = check_result_store->append();
            Gtk::TreeModel::Row row = *iter;
            row[tree_columns.name] = rule_descriptions.at(rule_id).name;
            row[tree_columns.running] = true;
            row[tree_columns.has_location] = false;
            run_store.emplace(rule_id, row);

            std::thread thr(&RulesWindow::check_thread, this, rule_id);
            thr.detach();
        }
    }
    run_button->set_sensitive(false);
    apply_button->set_sensitive(false);
    set_modal(true);
    stack->set_visible_child("checks");
    stack_switcher->set_sensitive(false);

    dynamic_cast<Gtk::HeaderBar *>(get_titlebar())->set_show_close_button(false);
}

void RulesWindow::rule_selected(RuleID id)
{
    auto multi = rule_descriptions.at(id).multi;
    rev_multi->set_reveal_child(multi);
    rule_id_current = id;
    if (multi) {
        show_editor(nullptr);
        update_rule_instances(id);
        update_warning();
    }
    else {
        auto rule = rules.get_rule(id);
        show_editor(create_editor(*rule));
    }
}

void RulesWindow::rule_instance_selected(RuleID id, const UUID &uu)
{
    auto rule = rules.get_rule(id, uu);
    if (rule == nullptr)
        return;
    show_editor(create_editor(*rule));
}

void RulesWindow::reload_editor()
{
    if (editor)
        show_editor(create_editor(editor->get_rule()));
}

void RulesWindow::show_editor(RuleEditor *e)
{
    if (editor) {
        delete editor;
        editor = nullptr;
    }
    if (e == nullptr)
        return;
    editor = Gtk::manage(e);
    editor->set_sensitive(enabled);
    rule_editor_box->pack_start(*editor, true, true, 0);
    editor->show();
    editor->signal_updated().connect([this] {
        update_rule_instance_labels();
        update_rule_labels();
        update_warning();
        s_signal_changed.emit();
    });
}

RuleEditor *RulesWindow::create_editor(Rule &r)
{
    RuleEditor *e = nullptr;
    switch (r.id) {
    case RuleID::HOLE_SIZE:
        e = new RuleEditorHoleSize(r, core);
        break;

    case RuleID::CLEARANCE_SILKSCREEN_EXPOSED_COPPER:
    case RuleID::PARAMETERS:
    case RuleID::CLEARANCE_PACKAGE:
        e = new RuleEditorClearanceSilkscreenExposedCopper(r, core);
        break;

    case RuleID::TRACK_WIDTH:
        e = new RuleEditorTrackWidth(r, core);
        break;

    case RuleID::CLEARANCE_COPPER:
        e = new RuleEditorClearanceCopper(r, core);
        break;

    case RuleID::SINGLE_PIN_NET:
        e = new RuleEditorSinglePinNet(r, core);
        break;

    case RuleID::VIA:
        e = new RuleEditorVia(r, core);
        break;

    case RuleID::CLEARANCE_COPPER_OTHER:
        e = new RuleEditorClearanceCopperOther(r, core);
        break;

    case RuleID::PLANE:
        e = new RuleEditorPlane(r, core);
        break;

    case RuleID::DIFFPAIR:
        e = new RuleEditorDiffpair(r, core);
        break;

    case RuleID::PACKAGE_CHECKS:
    case RuleID::PREFLIGHT_CHECKS:
    case RuleID::SYMBOL_CHECKS:
        e = new RuleEditorPackageChecks(r, core);
        break;

    case RuleID::CLEARANCE_COPPER_KEEPOUT:
        e = new RuleEditorClearanceCopperKeepout(r, core);
        break;

    case RuleID::LAYER_PAIR:
        e = new RuleEditorLayerPair(r, core);
        break;

    case RuleID::CLEARANCE_SAME_NET:
        e = new RuleEditorClearanceSameNet(r, core);
        break;

    case RuleID::SHORTED_PADS:
        e = new RuleEditorShortedPads(r, core);
        break;

    default:
        e = new RuleEditor(r, core);
    }
    e->populate();
    return e;
}

Block *RulesWindow::get_top_block()
{
    return core.get_top_block();
}

void RulesWindow::update_rule_instances(RuleID id)
{
    auto inst = rules.get_rules(id);
    auto row = lb_multi->get_selected_row();
    UUID uu;
    if (row) {
        uu = dynamic_cast<RuleLabel *>(row->get_child())->uuid;
    }
    for (auto it : lb_multi->get_children()) {
        delete it;
    }
    for (const auto &it : inst) {
        auto la = Gtk::manage(new RuleLabel(sg_order, it.second->get_brief(get_top_block(), &core.get_pool()), id,
                                            it.first, it.second->get_order()));
        la->set_sensitive(it.second->enabled);
        la->set_imported(it.second->imported);
        la->show();
        lb_multi->append(*la);
        if (it.first == uu) {
            auto this_row = dynamic_cast<Gtk::ListBoxRow *>(la->get_parent());
            lb_multi->select_row(*this_row);
        }
    }
    auto children = lb_multi->get_children();
    if (lb_multi->get_selected_row() == nullptr && children.size() > 0) {
        lb_multi->select_row(*static_cast<Gtk::ListBoxRow *>(children.front()));
    }
}

void RulesWindow::update_rule_instance_labels()
{
    for (auto ch : lb_multi->get_children()) {
        auto la = dynamic_cast<RuleLabel *>(dynamic_cast<Gtk::ListBoxRow *>(ch)->get_child());
        auto rule = rules.get_rule(la->id, la->uuid);
        la->set_text(rule->get_brief(get_top_block(), &core.get_pool()));
        la->set_sensitive(rule->enabled);
        la->set_order(rule->get_order());
        la->set_imported(rule->imported);
    }
    lb_multi->invalidate_sort();
}

void RulesWindow::update_rule_labels()
{
    for (auto ch : lb_rules->get_children()) {
        auto la = dynamic_cast<RuleLabel *>(dynamic_cast<Gtk::ListBoxRow *>(ch)->get_child());
        auto is_multi = rule_descriptions.at(la->id).multi;
        if (!is_multi) {
            auto r = rules.get_rule(la->id);
            la->set_sensitive(r->enabled);
            la->set_imported(r->imported);
        }
        else {
            auto r = rules.get_rules(la->id);
            auto imported = std::any_of(r.begin(), r.end(), [](const auto x) { return x.second->imported; });
            la->set_imported(imported);
        }
    }
}

void RulesWindow::set_enabled(bool enable)
{
    apply_button->set_sensitive(enable);
    run_button->set_sensitive(enable);
    if (editor)
        editor->set_sensitive(enable);
    button_rule_instance_add->set_sensitive(enable);
    button_rule_instance_remove->set_sensitive(enable);
    button_rule_instance_move_down->set_sensitive(enable);
    button_rule_instance_move_up->set_sensitive(enable);
    enabled = enable;
}

void RulesWindow::update_warning()
{
    if (!rule_descriptions.at(rule_id_current).needs_match_all) {
        rev_warn->set_reveal_child(false);
        return;
    }

    auto sorted = rules.get_rules_sorted(rule_id_current);
    if (sorted.size() == 0) {
        rev_warn->set_reveal_child(true);
    }
    else {
        const auto &last_rule = sorted.back();
        rev_warn->set_reveal_child(!last_rule->enabled || !last_rule->is_match_all());
    }
}

RulesWindow *RulesWindow::create(Gtk::Window *p, CanvasGL &ca, Core &c)
{
    RulesWindow *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/horizon-eda/horizon/imp/rules/rules_window.ui");
    x->get_widget_derived("window", w, ca, c);
    w->set_transient_for(*p);
    return w;
}
} // namespace horizon
