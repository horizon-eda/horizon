#include "imp_schematic.hpp"
#include "canvas/canvas_gl.hpp"
#include "export_pdf/export_pdf.hpp"
#include "pool/part.hpp"
#include "rules/rules_window.hpp"
#include "widgets/sheet_box.hpp"
#include "util/gtk_util.hpp"
#include "util/util.hpp"
#include "util/str_util.hpp"
#include "util/selection_util.hpp"
#include "bom_export_window.hpp"
#include "pdf_export_window.hpp"
#include "nlohmann/json.hpp"
#include "core/tools/tool_backannotate_connection_lines.hpp"
#include "core/tools/tool_add_part.hpp"
#include "core/tools/tool_map_symbol.hpp"
#include "widgets/unplaced_box.hpp"
#include "core/tool_id.hpp"
#include "actions.hpp"
#include "widgets/action_button.hpp"
#include <iostream>

namespace horizon {
ImpSchematic::ImpSchematic(const std::string &blocks_filename, const std::string &pictures_dir,
                           const PoolParams &pool_params)
    : ImpBase(pool_params), core_schematic(blocks_filename, pictures_dir, *pool, *pool_caching),
      project_dir(Glib::path_get_dirname(blocks_filename)), searcher(core_schematic)
{
    core = &core_schematic;
    core_schematic.signal_tool_changed().connect(sigc::mem_fun(*this, &ImpSchematic::handle_tool_change));
    core_schematic.signal_rebuilt().connect(sigc::mem_fun(*this, &ImpSchematic::handle_core_rebuilt));
}

void ImpSchematic::canvas_update()
{
    if (core_schematic.get_block_symbol_mode()) {
        canvas->update(core_schematic.get_canvas_data_block_symbol());
        warnings_box->update({});
    }
    else {
        canvas->update(core_schematic.get_canvas_data());
        warnings_box->update(core_schematic.get_sheet()->warnings);
        update_highlights();
    }
}

void ImpSchematic::handle_select_sheet(const UUID &sheet, const UUID &block, const UUIDVec &instance_path)
{
    if (sheet == current_sheet && block == core_schematic.get_current_block_uuid()
        && instance_path == core_schematic.get_instance_path())
        return;

    {
        auto [sc, offset] = canvas->get_scale_and_offset();
        view_infos[{current_sheet, core_schematic.get_current_block_uuid()}] =
                ViewInfo{sc, offset, canvas->get_selection()};
    }
    auto highlights_saved = highlights;
    auto highlights_hierarchical_saved = highlights_hierarchical;

    core_schematic.set_block(block);
    if (sheet) { // actual sheet
        core_schematic.set_sheet(sheet);
        core_schematic.set_instance_path(instance_path);
        canvas->markers.set_sheet_filter(uuid_vec_append(instance_path, sheet));
        unplaced_box->set_title("Symbol");
    }
    else {
        core_schematic.set_block_symbol_mode();
        unplaced_box->set_title("Port");
    }
    update_unplaced();
    canvas_update();
    update_action_sensitivity();
    update_instance_path_bar();


    if (const auto k = std::make_pair(sheet, block); view_infos.count(k)) {
        const auto &inf = view_infos.at(k);
        canvas->set_scale_and_offset(inf.scale, inf.offset);
        canvas->set_selection(inf.selection);
    }
    else {
        auto bbox = canvas->get_bbox();
        canvas->zoom_to_bbox(bbox.first, bbox.second);

        canvas->set_selection({});
    }

    highlights = highlights_saved;
    highlights_hierarchical = highlights_hierarchical_saved;
    update_highlights();

    for (auto b : action_buttons_schematic) {
        b->set_visible(sheet);
    }
    current_sheet = sheet;
}

static const UUID initial_path_for_bar = UUID("9b98ab12-3937-4c41-98b0-57994dbbfc59");

static bool is_prefix(const UUIDVec &path, const UUIDVec &path_for_bar)
{
    if (path_for_bar.size() && path_for_bar.front() == initial_path_for_bar)
        return false;
    if (path.size() > path_for_bar.size())
        return false;
    const auto len = path.size();
    return std::equal(path.begin(), path.begin() + len, path_for_bar.begin(), path_for_bar.begin() + len);
}

class BlockButton : public Gtk::Button {
public:
    BlockButton(const std::string &la, const UUIDVec &p) : Gtk::Button(la), path(p)
    {
        set_focus_on_click(false);
        signal_clicked().connect([this] { s_signal_selected.emit(path, sheet); });
        signal_enter_notify_event().connect([this](GdkEventCrossing *ev) -> bool {
            get_window()->set_cursor(Gdk::Cursor::create(Gdk::Display::get_default(), "pointer"));
            return false;
        });
        signal_leave_notify_event().connect([this](GdkEventCrossing *ev) -> bool {
            get_window()->set_cursor();
            return false;
        });
    }
    const UUIDVec path;
    UUID sheet;

    void set_active(bool v)
    {
        if (v)
            get_style_context()->add_class("active");
        else
            get_style_context()->remove_class("active");
    }

    typedef sigc::signal<void, UUIDVec, UUID> type_signal_selected;
    type_signal_selected signal_selected()
    {
        return s_signal_selected;
    }

private:
    type_signal_selected s_signal_selected;
};

static std::optional<std::string> instance_path_to_string(const Block &top, const UUIDVec &path)
{
    const Block *block = &top;
    std::string name = "Top";
    for (const auto &uu : path) {
        if (block->block_instances.count(uu)) {
            auto &inst = block->block_instances.at(uu);
            name = inst.refdes;
            block = inst.block;
        }
        else {
            return {};
        }
    }
    return name;
}

void ImpSchematic::update_instance_path_bar()
{
    const auto &top = *core_schematic.get_top_block();
    main_window->instance_path_revealer->set_reveal_child(core_schematic.get_blocks().blocks.size() > 1);
    if (!core_schematic.in_hierarchy()) {
        main_window->hierarchy_stack->set_visible_child("out_of_hierarchy");
        const auto sym_mode = core_schematic.get_block_symbol_mode();
        main_window->block_symbol_button->set_visible(!sym_mode);
        main_window->ports_button->set_visible(true);
        main_window->block_symbol_button->set_sensitive(true);
        main_window->ports_button->set_sensitive(true);
        if (sym_mode) {
            main_window->out_of_hierarchy_label->set_text("Editing block symbol");
        }
        else {
            main_window->out_of_hierarchy_label->set_text("Editing block outside of hierarchy");
        }

        return;
    }
    main_window->hierarchy_stack->set_visible_child("in_hierarchy");
    main_window->block_symbol_button->set_visible(true);
    main_window->ports_button->set_visible(true);

    // if instance path isn't a prefix of instance_path_for_bar, reset it
    const auto path = core_schematic.get_instance_path();

    {
        const bool not_top = path.size();
        main_window->parent_block_button->set_sensitive(not_top);
        main_window->block_symbol_button->set_sensitive(not_top);
        main_window->ports_button->set_sensitive(not_top);
    }
    if (!is_prefix(path, instance_path_for_bar)) {
        instance_path_for_bar = path;
        for (auto ch : main_window->instance_path_box->get_children()) {
            delete ch;
        }
        const auto *block = &top;
        UUIDVec current_path;
        auto add_button = [this, &current_path, &path](const std::string &label, const UUIDVec &pa) {
            auto bu = Gtk::manage(new BlockButton(label, pa));
            if (current_path == path) {
                bu->set_active(true);
                bu->sheet = core_schematic.get_sheet()->uuid;
            }
            bu->show();
            bu->signal_selected().connect(
                    [this](const auto p, const auto sheet) { sheet_box->go_to_instance(p, sheet); });
            main_window->instance_path_box->pack_start(*bu, false, false, 0);
        };
        add_button("Top", {});
        for (const auto &it : instance_path_for_bar) {
            {
                auto la = Gtk::manage(new Gtk::Label("/"));
                la->show();
                main_window->instance_path_box->pack_start(*la, false, false, 0);
            }
            current_path.push_back(it);
            const auto &inst = block->block_instances.at(it);
            add_button(inst.refdes, current_path);
            block = inst.block;
        }
    }
    else {
        size_t valid_len = 0;
        for (auto ch : main_window->instance_path_box->get_children()) {
            if (auto bu = dynamic_cast<BlockButton *>(ch)) {
                if (auto name = instance_path_to_string(top, bu->path)) {
                    bu->set_active(bu->path == path);
                    if (bu->path == path) {
                        bu->sheet = core_schematic.get_sheet()->uuid;
                    }
                    bu->set_label(*name);
                    valid_len = std::max(valid_len, bu->path.size());
                }
                else {
                    delete bu;
                }
            }
        }
        assert(valid_len <= instance_path_for_bar.size());
        instance_path_for_bar.resize(valid_len);
    }
}

void ImpSchematic::handle_next_prev_sheet(const ActionConnection &conn)
{
    const auto &sch = *core_schematic.get_current_schematic();
    const int sheet_idx_current = core_schematic.get_sheet()->index;
    const int inc = conn.id.action == ActionID::PREV_SHEET ? -1 : 1;
    const int sheet_idx_next = sheet_idx_current + inc;
    auto next_sheet = std::find_if(sch.sheets.begin(), sch.sheets.end(),
                                   [sheet_idx_next](const auto &x) { return (int)x.second.index == sheet_idx_next; });
    if (next_sheet != sch.sheets.end()) {
        sheet_box->select_sheet(next_sheet->second.uuid);
    }
    else if (conn.id.action == ActionID::PREV_SHEET && core_schematic.get_instance_path().size()) {
        trigger_action(ActionID::POP_OUT_OF_BLOCK);
    }
}

static void add_net_highlight(std::set<ObjectRef> &highlights_for_sheet, const Sheet &sheet, const UUID &net)
{
    for (const auto &[uu, line] : sheet.net_lines) {
        if (line.net.uuid == net) {
            highlights_for_sheet.emplace(ObjectType::LINE_NET, uu);
        }
    }
    for (const auto &[uu, junc] : sheet.junctions) {
        if (junc.net.uuid == net) {
            highlights_for_sheet.emplace(ObjectType::JUNCTION, uu);
        }
    }
    for (const auto &[uu, label] : sheet.net_labels) {
        if (label.junction->net.uuid == net) {
            highlights_for_sheet.emplace(ObjectType::NET_LABEL, uu);
        }
    }
    for (const auto &[uu, sym] : sheet.power_symbols) {
        if (sym.junction->net.uuid == net) {
            highlights_for_sheet.emplace(ObjectType::POWER_SYMBOL, uu);
        }
    }
    for (const auto &[uu_sym, sym] : sheet.symbols) {
        auto &component = *sym.component;
        for (const auto &[uu_pin, pin] : sym.symbol.pins) {
            UUIDPath<2> connpath(sym.gate->uuid, uu_pin);
            if (component.connections.count(connpath)) {
                auto net_conn = component.connections.at(connpath).net;
                if (net_conn.uuid == net) {
                    highlights_for_sheet.emplace(ObjectType::SYMBOL_PIN, uu_pin, uu_sym);
                }
            }
        }
    }
    for (const auto &[uu_sym, sym] : sheet.block_symbols) {
        auto &inst = *sym.block_instance;
        for (const auto &[uu_port, port] : sym.symbol.ports) {
            if (inst.connections.count(uu_port)) {
                auto net_conn = inst.connections.at(uu_port).net;
                if (net_conn.uuid == net) {
                    highlights_for_sheet.emplace(ObjectType::BLOCK_SYMBOL_PORT, uu_port, uu_sym);
                }
            }
        }
    }
}

void ImpSchematic::update_highlights()
{
    if (core_schematic.get_block_symbol_mode()) {
        canvas->set_highlight_enabled(false);
        return;
    }
    sheet_box->clear_highlights();
    for (const auto &it_sheet : core_schematic.get_top_schematic()->get_all_sheets()) {
        const auto &sheet = it_sheet.sheet;
        std::set<ObjectRef> highlights_for_sheet;
        for (const auto &it : highlights) {
            if (it.type == ObjectType::NET) {
                add_net_highlight(highlights_for_sheet, sheet, it.uuid);
            }
            else if (it.type == ObjectType::COMPONENT) {
                for (const auto &it_sym : sheet.symbols) {
                    auto component = it_sym.second.component;
                    if (uuid_vec_flatten(uuid_vec_append(it_sheet.instance_path, component->uuid)) == it.uuid) {
                        highlights_for_sheet.emplace(ObjectType::SCHEMATIC_SYMBOL, it_sym.first);
                    }
                }
            }
            else if (it.type == ObjectType::SCHEMATIC_SYMBOL) {
                if (sheet.symbols.count(it.uuid))
                    highlights_for_sheet.emplace(it);
            }
            else if (it.type == ObjectType::SYMBOL_PIN) {
                if (sheet.symbols.count(it.uuid2))
                    highlights_for_sheet.emplace(it);
            }
        }
        for (const auto &it : highlights_hierarchical) {
            if (it.instance_path != it_sheet.instance_path)
                continue;
            if (it.type == ObjectType::NET) {
                add_net_highlight(highlights_for_sheet, sheet, it.uuid);
            }
        }
        if (highlights_for_sheet.size())
            sheet_box->add_highlights(sheet.uuid, it_sheet.instance_path);
        if (it_sheet.sheet.uuid == core_schematic.get_sheet()->uuid
            && it_sheet.instance_path == core_schematic.get_instance_path()) {
            canvas->set_flags_all(0, TriangleInfo::FLAG_HIGHLIGHT);
            canvas->set_highlight_enabled(highlights_for_sheet.size());
            for (const auto &it : highlights_for_sheet) {
                canvas->set_flags(it, TriangleInfo::FLAG_HIGHLIGHT, 0);
            }
        }
    }
}

void ImpSchematic::clear_highlights()
{
    highlights.clear();
    highlights_hierarchical.clear();
}

bool ImpSchematic::handle_broadcast(const json &j)
{
    if (!ImpBase::handle_broadcast(j)) {
        const auto op = j.at("op").get<std::string>();
        guint32 timestamp = j.value("time", 0);
        if (op == "place-part") {
            main_window->present(timestamp);
            if (force_end_tool()) {
                auto data = std::make_unique<ToolAddPart::ToolDataAddPart>(j.at("part").get<std::string>());
                tool_begin(ToolID::ADD_PART, false, {}, std::move(data));
            }
        }
        else if (op == "assign-part" && !core->tool_is_active()) {
            main_window->present(timestamp);
            if (force_end_tool()) {
                auto data = std::make_unique<ToolAddPart::ToolDataAddPart>(j.at("part").get<std::string>());
                tool_begin(ToolID::ASSIGN_PART, false, {}, std::move(data));
            }
        }
        else if (op == "highlight" && cross_probing_enabled && !core->tool_is_active()) {
            clear_highlights();
            const json &o = j["objects"];
            for (auto it = o.cbegin(); it != o.cend(); ++it) {
                auto type = static_cast<ObjectType>(it.value().at("type").get<int>());
                if (type == ObjectType::COMPONENT) {
                    UUID uu(it.value().at("uuid").get<std::string>());
                    highlights.emplace(type, uu);
                }
                else if (type == ObjectType::NET) {
                    for (const auto &path_string : it.value().at("uuid").get<std::vector<std::string>>()) {
                        const auto href = uuid_vec_from_string(path_string);
                        const auto [path, uu] = uuid_vec_split(href);
                        highlights_hierarchical.push_back(HighlightItem{type, uu, path});
                    }
                }
            }
            update_highlights();
        }
        else if (op == "backannotate") {
            auto data = std::make_unique<ToolBackannotateConnectionLines::ToolDataBackannotate>();
            const json &o = j.at("connections");
            for (auto it = o.cbegin(); it != o.cend(); ++it) {
                auto &v = it.value();
                std::pair<ToolBackannotateConnectionLines::ToolDataBackannotate::Item,
                          ToolBackannotateConnectionLines::ToolDataBackannotate::Item>
                        item;
                auto &block = *core_schematic.get_top_block();
                item.first.from_json(block, v.at("from"));
                item.second.from_json(block, v.at("to"));
                if (item.first.is_valid() && item.second.is_valid())
                    data->connections.push_back(item);
            }
            if (data->connections.size()) {
                main_window->present(timestamp);
                if (force_end_tool())
                    tool_begin(ToolID::BACKANNOTATE_CONNECTION_LINES, true, {}, std::move(data));
            }
        }
        else if (op == "edit-meta") {
            main_window->present(timestamp);
            if (force_end_tool())
                tool_begin(ToolID::EDIT_PROJECT_PROPERTIES);
        }
    }
    return true;
}

void ImpSchematic::handle_selection_cross_probe()
{
    if (core_schematic.get_block_symbol_mode())
        return;
    json j;
    j["op"] = "schematic-select";
    j["selection"] = nullptr;
    for (const auto &it : canvas->get_selection()) {
        json k;
        ObjectType type = ObjectType::INVALID;
        UUID uu;
        auto sheet = core_schematic.get_sheet();
        switch (it.type) {
        case ObjectType::LINE_NET: {
            auto &li = sheet->net_lines.at(it.uuid);
            if (li.net) {
                type = ObjectType::NET;
                uu = li.net->uuid;
            }
        } break;
        case ObjectType::NET_LABEL: {
            auto &la = sheet->net_labels.at(it.uuid);
            if (la.junction->net) {
                type = ObjectType::NET;
                uu = la.junction->net->uuid;
            }
        } break;
        case ObjectType::POWER_SYMBOL: {
            auto &sym = sheet->power_symbols.at(it.uuid);
            if (sym.junction->net) {
                type = ObjectType::NET;
                uu = sym.junction->net->uuid;
            }
        } break;
        case ObjectType::JUNCTION: {
            auto &ju = sheet->junctions.at(it.uuid);
            if (ju.net) {
                type = ObjectType::NET;
                uu = ju.net->uuid;
            }
        } break;
        case ObjectType::SCHEMATIC_SYMBOL: {
            auto &sym = sheet->symbols.at(it.uuid);
            type = ObjectType::COMPONENT;
            uu = uuid_vec_flatten(uuid_vec_append(core_schematic.get_instance_path(), sym.component->uuid));
        } break;
        default:;
        }

        if (type != ObjectType::INVALID) {
            k["type"] = static_cast<int>(type);
            if (type == ObjectType::COMPONENT) {
                k["uuid"] = (std::string)uu;
            }
            else if (type == ObjectType::NET) {
                k["uuid"] = uuid_vec_to_string(uuid_vec_append(core_schematic.get_instance_path(), uu));
            }
            j["selection"].push_back(k);
        }
    }
    send_json(j);
}

int ImpSchematic::get_board_pid()
{
    json j;
    j["op"] = "get-board-pid";
    return this->send_json(j).get<int>();
}

void ImpSchematic::handle_show_in_pool_manager(const ActionConnection &conn)
{
    const auto &sheet = *core_schematic.get_sheet();
    const auto pool_sel = conn.id.action == ActionID::SHOW_IN_PROJECT_POOL_MANAGER ? ShowInPoolManagerPool::CURRENT
                                                                                   : ShowInPoolManagerPool::LAST;
    for (const auto &it : canvas->get_selection()) {
        if (it.type == ObjectType::SCHEMATIC_SYMBOL) {
            const auto &sym = sheet.symbols.at(it.uuid);
            if (sym.component->part) {
                show_in_pool_manager(ObjectType::PART, sym.component->part->uuid, pool_sel);
                break;
            }
        }
    }
}

static void block_to_board(json &j, const Block &block, UUIDVec instance_path)
{
    if (Block::instance_path_too_long(instance_path, __FUNCTION__))
        return;
    for (const auto &[uu, comp] : block.components) {
        json k;
        k["type"] = static_cast<int>(ObjectType::COMPONENT);
        k["uuid"] = (std::string)uuid_vec_flatten(uuid_vec_append(instance_path, uu));
        j.push_back(k);
    }
    for (const auto &[uu, inst] : block.block_instances) {
        block_to_board(j, *inst.block, uuid_vec_append(instance_path, uu));
    }
}

void ImpSchematic::construct()
{
    state_store = std::make_unique<WindowStateStore>(main_window, "imp-schematic");

    instance_path_for_bar = {initial_path_for_bar}; // make sure it's reset

    sheet_box = Gtk::manage(new SheetBox(core_schematic));
    sheet_box->show_all();
    sheet_box->signal_select_sheet().connect(sigc::mem_fun(*this, &ImpSchematic::handle_select_sheet));
    sheet_box->signal_edit_more().connect([this] { trigger_action(ToolID::EDIT_SCHEMATIC_PROPERTIES); });
    sheet_box->signal_place_block().connect([this](UUID block) {
        std::set<SelectableRef> sel;
        sel.emplace(block, ObjectType::BLOCK);
        this->tool_begin(ToolID::ADD_BLOCK_INSTANCE, true, sel);
    });

    main_window->left_panel->pack_start(*sheet_box, false, false, 0);

    main_window->parent_block_button->signal_clicked().connect([this] { trigger_action(ActionID::POP_OUT_OF_BLOCK); });
    main_window->block_symbol_button->signal_clicked().connect(
            [this] { trigger_action(ActionID::GO_TO_BLOCK_SYMBOL); });
    main_window->ports_button->signal_clicked().connect([this] { trigger_action(ToolID::MANAGE_PORTS); });

    hamburger_menu->append("Annotate…", "win.annotate");
    add_tool_action(ToolID::ANNOTATE, "annotate");

    hamburger_menu->append("Buses", "win.manage_buses");
    add_tool_action(ToolID::MANAGE_BUSES, "manage_buses");

    hamburger_menu->append("Net classes", "win.manage_nc");
    add_tool_action(ToolID::MANAGE_NET_CLASSES, "manage_nc");

    hamburger_menu->append("Power Nets", "win.manage_pn");
    add_tool_action(ToolID::MANAGE_POWER_NETS, "manage_pn");

    hamburger_menu->append("Sheets and blocks", "win.sch_properties");
    add_tool_action(ToolID::EDIT_SCHEMATIC_PROPERTIES, "sch_properties");

    hamburger_menu->append("Project properties", "win.prj_properties");
    add_tool_action(ToolID::EDIT_PROJECT_PROPERTIES, "prj_properties");

    hamburger_menu->append("Export BOM…", "win.bom_export");
    main_window->add_action("bom_export", [this] { trigger_action(ActionID::BOM_EXPORT_WINDOW); });

    hamburger_menu->append("Export PDF…", "win.export_pdf");
    main_window->add_action("export_pdf", [this] { trigger_action(ActionID::PDF_EXPORT_WINDOW); });

    if (sockets_connected) {
        hamburger_menu->append("Cross probing", "win.cross_probing");
        auto cp_action = main_window->add_action_bool("cross_probing", true);
        cross_probing_enabled = true;
        cp_action->signal_change_state().connect([this, cp_action](const Glib::VariantBase &v) {
            cross_probing_enabled = Glib::VariantBase::cast_dynamic<Glib::Variant<bool>>(v).get();
            g_simple_action_set_state(cp_action->gobj(), g_variant_new_boolean(cross_probing_enabled));
            if (!cross_probing_enabled && !core_schematic.tool_is_active()) {
                clear_highlights();
                update_highlights();
            }
        });
    }

    canvas->set_highlight_mode(CanvasGL::HighlightMode::DIM);

    connect_action(ActionID::PLACE_PART, [this](const auto &conn) {
        if (!sockets_connected) {
            this->tool_begin(ToolID::ADD_PART);
        }
        else {
            json j;
            j["op"] = "show-browser";
            j["time"] = gtk_get_current_event_time();
            allow_set_foreground_window(mgr_pid);
            this->send_json(j);
        }
    });

    connect_action(ActionID::PREV_SHEET, sigc::mem_fun(*this, &ImpSchematic::handle_next_prev_sheet));
    connect_action(ActionID::NEXT_SHEET, sigc::mem_fun(*this, &ImpSchematic::handle_next_prev_sheet));

    if (sockets_connected) {
        connect_action(ActionID::TO_BOARD, [this](const auto &conn) {
            json j;
            j["op"] = "to-board";
            j["time"] = gtk_get_current_event_time();
            j["selection"] = nullptr;
            for (const auto &it : canvas->get_selection()) {
                auto sheet = core_schematic.get_sheet();
                if (it.type == ObjectType::SCHEMATIC_SYMBOL) {
                    const auto &sym = sheet->symbols.at(it.uuid);
                    json k;
                    k["type"] = static_cast<int>(ObjectType::COMPONENT);
                    k["uuid"] = (std::string)uuid_vec_flatten(
                            uuid_vec_append(core_schematic.get_instance_path(), sym.component->uuid));
                    j["selection"].push_back(k);
                }
                else if (it.type == ObjectType::SCHEMATIC_BLOCK_SYMBOL) {
                    const auto &sym = sheet->block_symbols.at(it.uuid);
                    block_to_board(j["selection"], *sym.block_instance->block,
                                   uuid_vec_append(core_schematic.get_instance_path(), sym.block_instance->uuid));
                }
            }
            canvas->set_selection({});
            allow_set_foreground_window(this->get_board_pid());
            this->send_json(j);
        });
        set_action_sensitive(ActionID::TO_BOARD, false);

        connect_action(ActionID::SHOW_IN_BROWSER, [this](const auto &conn) {
            for (const auto &it : canvas->get_selection()) {
                auto sheet = core_schematic.get_sheet();
                if (it.type == ObjectType::SCHEMATIC_SYMBOL) {
                    auto &sym = sheet->symbols.at(it.uuid);
                    if (sym.component->part) {
                        auto ev = gtk_get_current_event();
                        json j;
                        j["op"] = "show-in-browser";
                        j["part"] = (std::string)sym.component->part->uuid;
                        j["time"] = gdk_event_get_time(ev);
                        this->send_json(j);
                        break;
                    }
                }
            }
        });

        connect_action(ActionID::SAVE_RELOAD_NETLIST, [this](const auto &conn) {
            this->trigger_action(ActionID::SAVE);
            json j;
            j["time"] = gtk_get_current_event_time();
            j["op"] = "reload-netlist";
            this->send_json(j);
        });
        set_action_sensitive(ActionID::SAVE_RELOAD_NETLIST, false);

        connect_action(ActionID::GO_TO_BOARD, [this](const auto &conn) {
            json j;
            j["time"] = gtk_get_current_event_time();
            j["op"] = "present-board";
            auto board_pid = this->get_board_pid();
            if (board_pid != -1)
                allow_set_foreground_window(board_pid);
            this->send_json(j);
        });

        connect_go_to_project_manager_action();


        connect_action(ActionID::SHOW_IN_POOL_MANAGER,
                       sigc::mem_fun(*this, &ImpSchematic::handle_show_in_pool_manager));
        connect_action(ActionID::SHOW_IN_PROJECT_POOL_MANAGER,
                       sigc::mem_fun(*this, &ImpSchematic::handle_show_in_pool_manager));
        set_action_sensitive(ActionID::SHOW_IN_POOL_MANAGER, false);
        set_action_sensitive(ActionID::SHOW_IN_PROJECT_POOL_MANAGER, false);
    }

    connect_action(ActionID::MOVE_TO_OTHER_SHEET,
                   std::bind(&ImpSchematic::handle_move_to_other_sheet, this, std::placeholders::_1));

    connect_action(ActionID::HIGHLIGHT_NET, [this](const auto &a) {
        clear_highlights();
        const auto flat = core_schematic.get_top_block()->flatten();
        const auto path = core_schematic.get_instance_path();
        for (const auto &it : canvas->get_selection()) {
            if (auto uu = net_from_selectable(it)) {
                const auto selected_net_path = uuid_vec_append(path, uu);
                for (const auto &[uu_net, net] : flat.nets) {
                    if (std::any_of(net.hrefs.begin(), net.hrefs.end(),
                                    [&selected_net_path](const auto &x) { return x == selected_net_path; })) {
                        for (auto &href : net.hrefs) {
                            const auto [net_path, hnet] = uuid_vec_split(href);
                            highlights_hierarchical.push_back(HighlightItem{ObjectType::NET, hnet, net_path});
                        }
                        break;
                    }
                }
            }
        }
        this->update_highlights();
    });

    connect_action(ActionID::HIGHLIGHT_NET_CLASS, [this](const auto &a) {
        clear_highlights();
        const auto flat = core_schematic.get_top_block()->flatten();
        const auto path = core_schematic.get_instance_path();
        for (const auto &it : canvas->get_selection()) {
            if (auto uu = net_from_selectable(it)) {
                const auto net_class = core_schematic.get_current_block()->nets.at(uu).net_class->uuid;
                for (const auto &[uu_net, net] : flat.nets) {
                    if (net.net_class->uuid == net_class) {
                        for (auto &href : net.hrefs) {
                            const auto [net_path, hnet] = uuid_vec_split(href);
                            highlights_hierarchical.push_back(HighlightItem{ObjectType::NET, hnet, net_path});
                        }
                    }
                }
            }
        }
        this->update_highlights();
    });

    connect_action(ActionID::HIGHLIGHT_GROUP, sigc::mem_fun(*this, &ImpSchematic::handle_highlight_group_tag));
    connect_action(ActionID::HIGHLIGHT_TAG, sigc::mem_fun(*this, &ImpSchematic::handle_highlight_group_tag));

    bom_export_window = BOMExportWindow::create(main_window, core_schematic, *core_schematic.get_bom_export_settings(),
                                                *pool.get(), project_dir);

    connect_action(ActionID::BOM_EXPORT_WINDOW, [this](const auto &c) { bom_export_window->present(); });
    connect_action(ActionID::EXPORT_BOM, [this](const auto &c) {
        bom_export_window->present();
        bom_export_window->generate();
    });

    bom_export_window->signal_changed().connect([this] { core_schematic.set_needs_save(); });
    core->signal_tool_changed().connect([this](ToolID t) { bom_export_window->set_can_export(t == ToolID::NONE); });
    core->signal_rebuilt().connect([this] { bom_export_window->update(); });

    pdf_export_window = PDFExportWindow::create(main_window, core_schematic, *core_schematic.get_pdf_export_settings(),
                                                project_dir);
    pdf_export_window->signal_changed().connect([this] { core_schematic.set_needs_save(); });
    connect_action(ActionID::PDF_EXPORT_WINDOW, [this](const auto &c) { pdf_export_window->present(); });
    connect_action(ActionID::EXPORT_PDF, [this](const auto &c) {
        pdf_export_window->present();
        pdf_export_window->generate();
    });

    unplaced_box = Gtk::manage(new UnplacedBox("Symbol"));
    unplaced_box->show();
    main_window->left_panel->pack_end(*unplaced_box, true, true, 0);
    unplaced_box->signal_place().connect([this](const auto &items) {
        if (core_schematic.get_block_symbol_mode()) {
            std::set<SelectableRef> sel;
            for (const auto &it : items) {
                sel.emplace(it.at(0), ObjectType::BLOCK_SYMBOL_PORT);
            }
            this->tool_begin(ToolID::MAP_PORT, true, sel);
        }
        else {
            auto d = std::make_unique<ToolMapSymbol::ToolDataMapSymbol>(items);
            this->tool_begin(ToolID::MAP_SYMBOL, false, {}, std::move(d));
        }
    });
    core_schematic.signal_rebuilt().connect(sigc::mem_fun(*this, &ImpSchematic::update_unplaced));
    update_unplaced();
    core_schematic.signal_tool_changed().connect(
            [this](ToolID tool_id) { unplaced_box->set_sensitive(tool_id == ToolID::NONE); });

    grid_controller->disable();
    canvas->snap_to_targets = false;

    connect_action(ActionID::TOGGLE_SNAP_TO_TARGETS, [this](const ActionConnection &c) {
        canvas->snap_to_targets = !canvas->snap_to_targets;
        g_simple_action_set_state(toggle_snap_to_targets_action->gobj(),
                                  g_variant_new_boolean(canvas->snap_to_targets));
    });
    view_options_menu_append_action("Snap to targets", "win.snap_to_targets");

    toggle_snap_to_targets_action = main_window->add_action_bool("snap_to_targets", canvas->snap_to_targets);
    toggle_snap_to_targets_action->signal_change_state().connect([this](const Glib::VariantBase &v) {
        auto b = Glib::VariantBase::cast_dynamic<Glib::Variant<bool>>(v).get();
        if (b != canvas->snap_to_targets) {
            trigger_action(ActionID::TOGGLE_SNAP_TO_TARGETS);
        }
    });


    rules_window->signal_goto().connect([this](Coordi location, UUID sheet, UUIDVec instance_path) {
        sheet_box->go_to_instance(instance_path, sheet);
        canvas->center_and_zoom(location);
    });

    canvas->signal_motion_notify_event().connect([this](GdkEventMotion *ev) {
        if (target_drag_begin.type != ObjectType::INVALID) {
            handle_drag();
        }
        return false;
    });

    canvas->signal_button_release_event().connect([this](GdkEventButton *ev) {
        target_drag_begin = Target();
        return false;
    });

    main_window->signal_activate_hud_link().connect(
            [this](const std::string &url) {
                if (url.find("s:") == 0) {
                    std::string sheet = url.substr(2);
                    auto uuid = UUID(sheet);
                    auto sch = core_schematic.get_current_schematic();
                    if (sch->sheets.count(uuid)) {
                        sheet_box->select_sheet(uuid);
                    }
                    return true;
                }
                return false;
            },
            false);

    connect_action(ActionID::PUSH_INTO_BLOCK, [this](const auto &a) {
        auto &x = sel_find_one(canvas->get_selection(), ObjectType::SCHEMATIC_BLOCK_SYMBOL);
        sheet_box->go_to_instance(
                uuid_vec_append(core_schematic.get_instance_path(),
                                core_schematic.get_sheet()->block_symbols.at(x.uuid).block_instance->uuid));
    });

    connect_action(ActionID::EDIT_BLOCK_SYMBOL, [this](const auto &a) {
        auto &x = sel_find_one(canvas->get_selection(), ObjectType::SCHEMATIC_BLOCK_SYMBOL);
        sheet_box->go_to_block_symbol(core_schematic.get_sheet()->block_symbols.at(x.uuid).block_instance->block->uuid);
    });

    connect_action(ActionID::GO_TO_BLOCK_SYMBOL,
                   [this](const auto &a) { sheet_box->go_to_block_symbol(core_schematic.get_current_block()->uuid); });

    connect_action(ActionID::POP_OUT_OF_BLOCK, [this](const auto &a) {
        const auto path_current = core_schematic.get_instance_path();
        if (path_current.size() == 0)
            return;
        const auto [path, inst] = uuid_vec_split(path_current);

        const Block *block = core_schematic.get_top_block();
        for (const auto &uu : path) {
            block = block->block_instances.at(uu).block;
        }

        for (const auto &[uu, sheet] : core_schematic.get_blocks().get_schematic(block->uuid).sheets) {
            for (const auto &[uu_sym, sym] : sheet.block_symbols) {
                if (sym.block_instance->uuid == inst) {
                    sheet_box->go_to_instance(path, uu);
                    return;
                }
            }
        }
    });

    connect_action(ActionID::OPEN_DATASHEET, [this](const auto &a) {
        const auto x = sel_find_exactly_one(canvas->get_selection(), ObjectType::SCHEMATIC_SYMBOL);
        if (x && core_schematic.get_sheet()->symbols.count(x->uuid)) {
            auto part = core_schematic.get_sheet()->symbols.at(x->uuid).component->part;
            if (part && part->get_datasheet().size())
                gtk_show_uri_on_window(GTK_WINDOW(main_window->gobj()), part->get_datasheet().c_str(), GDK_CURRENT_TIME,
                                       NULL);
        }
    });

    add_action_button_schematic(ActionID::PLACE_PART);
    add_action_button_schematic(ToolID::DRAW_NET);
    {
        auto &b = add_action_button_schematic(ToolID::PLACE_NET_LABEL);
        b.add_action(ToolID::PLACE_BUS_LABEL);
        b.add_action(ToolID::PLACE_BUS_RIPPER);
    }
    add_action_button_schematic(ToolID::PLACE_POWER_SYMBOL);
    add_action_button_line();
    add_action_button(ToolID::PLACE_TEXT);

    update_monitor();
}

ActionButton &ImpSchematic::add_action_button_schematic(ActionToolID id)
{
    auto &b = add_action_button(id);
    action_buttons_schematic.push_back(&b);
    return b;
}

void ImpSchematic::update_action_sensitivity()
{
    auto sel = canvas->get_selection();
    const auto n_block_sym = sel_count_type(sel, ObjectType::SCHEMATIC_BLOCK_SYMBOL);
    if (sockets_connected) {
        json req;
        req["op"] = "has-board";
        const auto has_board = send_json(req).get<bool>();
        set_action_sensitive(ActionID::SAVE_RELOAD_NETLIST, has_board);
        auto n_sym = std::count_if(sel.begin(), sel.end(),
                                   [](const auto &x) { return x.type == ObjectType::SCHEMATIC_SYMBOL; });
        set_action_sensitive(ActionID::SHOW_IN_BROWSER, n_sym == 1);
        set_action_sensitive(ActionID::SHOW_IN_POOL_MANAGER, n_sym == 1);
        set_action_sensitive(ActionID::SHOW_IN_PROJECT_POOL_MANAGER, n_sym == 1);
        if (!has_board) {
            set_action_sensitive(ActionID::TO_BOARD, false);
        }
        else {
            set_action_sensitive(ActionID::TO_BOARD, (n_sym || n_block_sym) && core_schematic.in_hierarchy());
        }
    }
    else {
        set_action_sensitive(ActionID::TO_BOARD, false);
        set_action_sensitive(ActionID::SHOW_IN_BROWSER, false);
        set_action_sensitive(ActionID::SHOW_IN_POOL_MANAGER, false);
        set_action_sensitive(ActionID::SHOW_IN_PROJECT_POOL_MANAGER, false);
    }
    set_action_sensitive(ActionID::MOVE_TO_OTHER_SHEET, sel.size() > 0 && !core_schematic.get_block_symbol_mode());
    set_action_sensitive(ActionID::GO_TO_BOARD, sockets_connected);
    set_action_sensitive(ActionID::GO_TO_PROJECT_MANAGER, sockets_connected);

    const bool can_highlight_net = std::any_of(sel.begin(), sel.end(), [](const auto &x) {
        switch (x.type) {
        case ObjectType::LINE_NET:
        case ObjectType::NET_LABEL:
        case ObjectType::JUNCTION:
        case ObjectType::POWER_SYMBOL:
            return true;

        default:
            return false;
        }
    });
    set_action_sensitive(ActionID::HIGHLIGHT_NET, can_highlight_net);
    set_action_sensitive(ActionID::HIGHLIGHT_NET_CLASS, can_highlight_net);
    bool can_higlight_group = false;
    bool can_higlight_tag = false;
    if (sel.size() == 1 && (*sel.begin()).type == ObjectType::SCHEMATIC_SYMBOL) {
        auto uu = (*sel.begin()).uuid;
        if (core_schematic.get_sheet()->symbols.count(uu)) {
            auto comp = core_schematic.get_sheet()->symbols.at(uu).component;
            can_higlight_group = comp->group;
            can_higlight_tag = comp->tag;
        }
    }

    {
        const auto x = sel_find_exactly_one(sel, ObjectType::SCHEMATIC_SYMBOL);
        bool sensitive = false;
        if (x && core_schematic.get_sheet()->symbols.count(x->uuid)) {
            auto part = core_schematic.get_sheet()->symbols.at(x->uuid).component->part;
            sensitive = x && part && part->get_datasheet().size();
        }
        set_action_sensitive(ActionID::OPEN_DATASHEET, sensitive);
    }

    set_action_sensitive(ActionID::NEXT_SHEET, !core_schematic.get_block_symbol_mode());
    set_action_sensitive(ActionID::PREV_SHEET, !core_schematic.get_block_symbol_mode());

    set_action_sensitive(ActionID::HIGHLIGHT_GROUP, can_higlight_group);
    set_action_sensitive(ActionID::HIGHLIGHT_TAG, can_higlight_tag);

    const auto have_block_sym = n_block_sym == 1;
    set_action_sensitive(ActionID::PUSH_INTO_BLOCK, have_block_sym && core_schematic.in_hierarchy());
    set_action_sensitive(ActionID::EDIT_BLOCK_SYMBOL, have_block_sym);
    set_action_sensitive(ActionID::POP_OUT_OF_BLOCK, core_schematic.get_instance_path().size());
    set_action_sensitive(ActionID::GO_TO_BLOCK_SYMBOL, !core_schematic.current_block_is_top());
    ImpBase::update_action_sensitivity();
}

const Block &ImpSchematic::get_block_for_group_tag_names()
{
    return *core_schematic.get_current_block();
}

std::map<ObjectType, ImpBase::SelectionFilterInfo> ImpSchematic::get_selection_filter_info() const
{
    std::map<ObjectType, ImpBase::SelectionFilterInfo> r;
    for (const auto &[type, desc] : object_descriptions) {
        switch (type) {
        case ObjectType::JUNCTION:
        case ObjectType::TEXT:
        case ObjectType::LINE:
        case ObjectType::ARC:
        case ObjectType::PICTURE:
        case ObjectType::SCHEMATIC_SYMBOL:
        case ObjectType::BUS_LABEL:
        case ObjectType::BUS_RIPPER:
        case ObjectType::NET_LABEL:
        case ObjectType::LINE_NET:
        case ObjectType::POWER_SYMBOL:
        case ObjectType::BLOCK_SYMBOL_PORT:
        case ObjectType::SCHEMATIC_BLOCK_SYMBOL:
        case ObjectType::SCHEMATIC_NET_TIE:
            r[type];
            break;
        default:;
        }
    }
    return r;
}

UUID ImpSchematic::net_from_selectable(const SelectableRef &sr)
{
    const auto &sheet = *core_schematic.get_sheet();
    switch (sr.type) {
    case ObjectType::LINE_NET: {
        auto &li = sheet.net_lines.at(sr.uuid);
        if (li.net) {
            return li.net->uuid;
        }
    } break;
    case ObjectType::NET_LABEL: {
        auto &la = sheet.net_labels.at(sr.uuid);
        if (la.junction->net) {
            return la.junction->net->uuid;
        }
    } break;
    case ObjectType::POWER_SYMBOL: {
        auto &sym = sheet.power_symbols.at(sr.uuid);
        if (sym.junction->net) {
            return sym.junction->net->uuid;
        }
    } break;
    case ObjectType::JUNCTION: {
        auto &ju = sheet.junctions.at(sr.uuid);
        if (ju.net) {
            return ju.net->uuid;
        }
    } break;
    default:;
    }
    return UUID();
}

std::string ImpSchematic::get_hud_text(std::set<SelectableRef> &sel)
{
    std::string s;
    if (auto it = sel_find_exactly_one(sel, ObjectType::SCHEMATIC_SYMBOL)) {
        const auto &comp = *core_schematic.get_sheet()->symbols.at(it->uuid).component;
        s += "<b>Symbol "
             + core_schematic.get_top_block()->get_component_info(comp, core_schematic.get_instance_path()).refdes
             + "</b>\n";
        s += get_hud_text_for_component(&comp);
        sel_erase_type(sel, ObjectType::SCHEMATIC_SYMBOL);
    }

    if (auto it = sel_find_exactly_one(sel, ObjectType::TEXT)) {
        const auto text = core->get_text(it->uuid);
        const auto txt = Glib::ustring(text->text);
        Glib::MatchInfo ma;
        if (core_schematic.get_current_schematic()->get_sheetref_regex()->match(txt, ma)) {
            s += "\n\n<b>Text with sheet references</b>\n";
            do {
                auto uuid = ma.fetch(1);
                std::string url = "s:" + uuid;
                if (core_schematic.get_current_schematic()->sheets.count(UUID(uuid))) {
                    s += make_link_markup(url, core_schematic.get_current_schematic()->sheets.at(UUID(uuid)).name);
                }
                else {
                    s += "Unknown Sheet";
                }
                s += "\n";
            } while (ma.next());
            sel_erase_type(sel, ObjectType::TEXT);
        }
    }
    trim(s);
    return s;
}

void ImpSchematic::handle_core_rebuilt()
{
    sheet_box->update();
    update_instance_path_bar();
}

void ImpSchematic::handle_tool_change(ToolID id)
{
    ImpBase::handle_tool_change(id);
    sheet_box->set_sensitive(id == ToolID::NONE);
    main_window->instance_path_revealer->set_sensitive(id == ToolID::NONE);
}

void ImpSchematic::handle_highlight_group_tag(const ActionConnection &a)
{
    const bool tag_mode = a.id.action == ActionID::HIGHLIGHT_TAG;
    auto sel = canvas->get_selection();
    if (sel.size() == 1 && (*sel.begin()).type == ObjectType::SCHEMATIC_SYMBOL) {
        auto comp_sel = core_schematic.get_sheet()->symbols.at((*sel.begin()).uuid).component;
        UUID uu = tag_mode ? comp_sel->tag : comp_sel->group;
        if (!uu)
            return;
        clear_highlights();
        for (const auto &[comp_uu, comp] : core_schematic.get_current_block()->components) {
            if ((tag_mode && (comp.tag == uu)) || (!tag_mode && (comp.group == uu))) {
                highlights.emplace(ObjectType::COMPONENT,
                                   uuid_vec_flatten(uuid_vec_append(core_schematic.get_instance_path(), comp_uu)));
            }
        }
        update_highlights();
    }
}

void ImpSchematic::handle_maybe_drag(bool ctrl)
{
    if (!preferences.schematic.drag_start_net_line) {
        ImpBase::handle_maybe_drag(ctrl);
        return;
    }

    auto target = canvas->get_current_target();
    if (target.type == ObjectType::SYMBOL_PIN || target.type == ObjectType::BLOCK_SYMBOL_PORT
        || target.type == ObjectType::JUNCTION) {
        std::cout << "click pin" << std::endl;
        canvas->inhibit_drag_selection();
        target_drag_begin = target;
        cursor_pos_drag_begin = canvas->get_cursor_pos_win();
    }
    else {
        ImpBase::handle_maybe_drag(ctrl);
    }
}

static bool drag_does_start(const Coordf &delta, Orientation orientation)
{
    float thr = 50;
    switch (orientation) {
    case Orientation::DOWN:
        return delta.y > thr;

    case Orientation::UP:
        return -delta.y > thr;

    case Orientation::RIGHT:
        return delta.x > thr;

    case Orientation::LEFT:
        return -delta.x > thr;

    default:
        return false;
    }
}

void ImpSchematic::handle_drag()
{
    auto pos = canvas->get_cursor_pos_win();
    auto delta = pos - cursor_pos_drag_begin;
    bool start = false;

    if (target_drag_begin.type == ObjectType::SYMBOL_PIN) {
        const auto &sym = core_schematic.get_sheet()->symbols.at(target_drag_begin.path.at(0));
        const auto &pin = sym.symbol.pins.at(target_drag_begin.path.at(1));
        auto orientation = pin.get_orientation_for_placement(sym.placement);
        start = drag_does_start(delta, orientation);
    }
    else if (target_drag_begin.type == ObjectType::BLOCK_SYMBOL_PORT) {
        const auto &sym = core_schematic.get_sheet()->block_symbols.at(target_drag_begin.path.at(0));
        const auto &port = sym.symbol.ports.at(target_drag_begin.path.at(1));
        auto orientation = port.get_orientation_for_placement(sym.placement);
        start = drag_does_start(delta, orientation);
    }
    else if (target_drag_begin.type == ObjectType::JUNCTION) {
        start = delta.mag_sq() > (50 * 50);
    }

    if (start) {
        {
            clear_highlights();
            update_highlights();
            ToolArgs args;
            args.coords = target_drag_begin.p;
            ToolResponse r = core->tool_begin(ToolID::DRAW_NET, args, imp_interface.get());
            tool_process(r);
        }
        {
            ToolArgs args;
            args.type = ToolEventType::ACTION;
            args.coords = target_drag_begin.p;
            args.action = InToolActionID::LMB;
            args.target = target_drag_begin;
            args.work_layer = canvas->property_work_layer();
            ToolResponse r = core->tool_update(args);
            tool_process(r);
        }
        target_drag_begin = Target();
    }
}

void ImpSchematic::search_center(const Searcher::SearchResult &res)
{
    if (res.sheet != core_schematic.get_sheet()->uuid || res.instance_path != core_schematic.get_instance_path()) {
        sheet_box->go_to_instance(res.instance_path, res.sheet);
    }
    ImpBase::search_center(res);
}

class SelectSheetDialog : public Gtk::Dialog {
public:
    SelectSheetDialog(const Schematic *sch, const Sheet *skip);
    UUID selected_sheet;

private:
    const Schematic *sch;
};

class MyLabel : public Gtk::Label {
public:
    MyLabel(const std::string &txt, const UUID &uu) : Gtk::Label(txt), uuid(uu)
    {
        set_xalign(0);
        property_margin() = 5;
    }

    const UUID uuid;
};

SelectSheetDialog::SelectSheetDialog(const Schematic *s, const Sheet *skip)
    : Gtk::Dialog("Select sheet", Gtk::DIALOG_MODAL | Gtk::DIALOG_USE_HEADER_BAR), sch(s)
{
    auto sc = Gtk::manage(new Gtk::ScrolledWindow);
    sc->set_propagate_natural_height(true);
    sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);

    auto lb = Gtk::manage(new Gtk::ListBox);
    lb->set_selection_mode(Gtk::SELECTION_NONE);
    lb->set_activate_on_single_click(true);
    lb->set_header_func(sigc::ptr_fun(header_func_separator));
    sc->add(*lb);

    for (const auto it : s->get_sheets_sorted()) {
        if (it != skip) {
            auto la = Gtk::manage(new MyLabel(std::to_string(it->index) + " " + it->name, it->uuid));
            lb->append(*la);
        }
    }
    lb->signal_row_activated().connect([this](Gtk::ListBoxRow *row) {
        auto la = dynamic_cast<MyLabel *>(row->get_child());
        selected_sheet = la->uuid;
        response(Gtk::RESPONSE_OK);
    });

    sc->show_all();
    get_content_area()->set_border_width(0);
    get_content_area()->pack_start(*sc, true, true, 0);
}

void ImpSchematic::handle_move_to_other_sheet(const ActionConnection &conn)
{
    bool added = true;
    auto selection = canvas->get_selection();
    // try to find everything that's connected to the selection in some way
    while (added) {
        std::set<SelectableRef> new_sel;
        for (const auto &it : selection) {
            switch (it.type) {
            case ObjectType::NET_LABEL: {
                auto &la = core_schematic.get_sheet()->net_labels.at(it.uuid);
                new_sel.emplace(la.junction->uuid, ObjectType::JUNCTION);
            } break;
            case ObjectType::BUS_LABEL: {
                auto &la = core_schematic.get_sheet()->bus_labels.at(it.uuid);
                new_sel.emplace(la.junction->uuid, ObjectType::JUNCTION);
            } break;
            case ObjectType::POWER_SYMBOL: {
                auto &ps = core_schematic.get_sheet()->power_symbols.at(it.uuid);
                new_sel.emplace(ps.junction->uuid, ObjectType::JUNCTION);
            } break;
            case ObjectType::BUS_RIPPER: {
                auto &rip = core_schematic.get_sheet()->bus_rippers.at(it.uuid);
                new_sel.emplace(rip.junction->uuid, ObjectType::JUNCTION);
            } break;
            case ObjectType::SCHEMATIC_NET_TIE: {
                auto &tie = core_schematic.get_sheet()->net_ties.at(it.uuid);
                new_sel.emplace(tie.from->uuid, ObjectType::JUNCTION);
                new_sel.emplace(tie.to->uuid, ObjectType::JUNCTION);
            } break;

            case ObjectType::LINE_NET: {
                auto line = &core_schematic.get_sheet()->net_lines.at(it.uuid);
                for (auto &it_ft : {line->from, line->to}) {
                    if (it_ft.is_junc()) {
                        new_sel.emplace(it_ft.junc->uuid, ObjectType::JUNCTION);
                    }
                    else if (it_ft.is_bus_ripper()) {
                        new_sel.emplace(it_ft.bus_ripper->uuid, ObjectType::BUS_RIPPER);
                    }
                    else if (it_ft.is_pin()) {
                        new_sel.emplace(it_ft.symbol->uuid, ObjectType::SCHEMATIC_SYMBOL);
                    }
                    else if (it_ft.is_port()) {
                        new_sel.emplace(it_ft.block_symbol->uuid, ObjectType::SCHEMATIC_BLOCK_SYMBOL);
                    }
                }
            } break;

            case ObjectType::LINE: {
                auto line = &core_schematic.get_sheet()->lines.at(it.uuid);
                for (auto &it_ft : {line->from, line->to}) {
                    new_sel.emplace(it_ft->uuid, ObjectType::JUNCTION);
                }
            } break;

            case ObjectType::ARC: {
                auto arc = &core_schematic.get_sheet()->arcs.at(it.uuid);
                for (auto &it_ft : {arc->from, arc->to, arc->center}) {
                    new_sel.emplace(it_ft->uuid, ObjectType::JUNCTION);
                }
            } break;

            case ObjectType::SCHEMATIC_SYMBOL: {
                auto &sym = core_schematic.get_sheet()->symbols.at(it.uuid);
                for (const auto &itt : sym.texts) {
                    new_sel.emplace(itt->uuid, ObjectType::TEXT);
                }
            } break;

            default:;
            }
        }

        // other direction
        for (const auto &it : core_schematic.get_sheet()->net_labels) {
            if (selection.count(SelectableRef(it.second.junction->uuid, ObjectType::JUNCTION))) {
                new_sel.emplace(it.first, ObjectType::NET_LABEL);
            }
        }
        for (const auto &it : core_schematic.get_sheet()->bus_labels) {
            if (selection.count(SelectableRef(it.second.junction->uuid, ObjectType::JUNCTION))) {
                new_sel.emplace(it.first, ObjectType::BUS_LABEL);
            }
        }
        for (const auto &it : core_schematic.get_sheet()->bus_rippers) {
            if (selection.count(SelectableRef(it.second.junction->uuid, ObjectType::JUNCTION))) {
                new_sel.emplace(it.first, ObjectType::BUS_RIPPER);
            }
        }
        for (const auto &it : core_schematic.get_sheet()->power_symbols) {
            if (selection.count(SelectableRef(it.second.junction->uuid, ObjectType::JUNCTION))) {
                new_sel.emplace(it.first, ObjectType::POWER_SYMBOL);
            }
        }
        for (const auto &it : core_schematic.get_sheet()->net_lines) {
            const auto line = it.second;
            bool add_line = false;
            for (auto &it_ft : {line.from, line.to}) {
                if (it_ft.is_junc()) {
                    if (selection.count(SelectableRef(it_ft.junc->uuid, ObjectType::JUNCTION))) {
                        add_line = true;
                    }
                }
                else if (it_ft.is_bus_ripper()) {
                    if (selection.count(SelectableRef(it_ft.bus_ripper->uuid, ObjectType::BUS_RIPPER))) {
                        add_line = true;
                    }
                }
                else if (it_ft.is_pin()) {
                    if (selection.count(SelectableRef(it_ft.symbol->uuid, ObjectType::SCHEMATIC_SYMBOL))) {
                        add_line = true;
                    }
                }
                else if (it_ft.is_port()) {
                    if (selection.count(SelectableRef(it_ft.block_symbol->uuid, ObjectType::SCHEMATIC_BLOCK_SYMBOL))) {
                        add_line = true;
                    }
                }
            }
            if (add_line) {
                new_sel.emplace(it.first, ObjectType::LINE_NET);
            }
        }
        for (const auto &it : core_schematic.get_sheet()->lines) {
            for (const auto &it_ft : {it.second.from, it.second.to}) {
                if (selection.count(SelectableRef(it_ft->uuid, ObjectType::JUNCTION))) {
                    new_sel.emplace(it.first, ObjectType::LINE);
                }
            }
        }
        for (const auto &it : core_schematic.get_sheet()->arcs) {
            for (const auto &it_ft : {it.second.from, it.second.to, it.second.center}) {
                if (selection.count(SelectableRef(it_ft->uuid, ObjectType::JUNCTION))) {
                    new_sel.emplace(it.first, ObjectType::ARC);
                }
            }
        }
        for (const auto &it : core_schematic.get_sheet()->net_ties) {
            for (const auto &it_ft : {it.second.from, it.second.to}) {
                if (selection.count(SelectableRef(it_ft->uuid, ObjectType::JUNCTION))) {
                    new_sel.emplace(it.first, ObjectType::SCHEMATIC_NET_TIE);
                }
            }
        }

        added = false;
        for (const auto &it : new_sel) {
            if (selection.insert(it).second) {
                added = true;
            }
        }
    }
    canvas->set_selection(selection);

    auto old_sheet = core_schematic.get_sheet();
    Sheet *new_sheet = nullptr;
    {
        SelectSheetDialog dia(core_schematic.get_current_schematic(), old_sheet);
        dia.set_transient_for(*main_window);
        if (dia.run() == Gtk::RESPONSE_OK) {
            new_sheet = &core_schematic.get_current_schematic()->sheets.at(dia.selected_sheet);
        }
    }
    if (!new_sheet)
        return;
    sheet_box->select_sheet(new_sheet->uuid);
    assert(core_schematic.get_sheet() == new_sheet);

    // actually move things to new sheet
    for (const auto &it : selection) {
        switch (it.type) {
        case ObjectType::NET_LABEL: {
            new_sheet->net_labels.insert(std::make_pair(it.uuid, std::move(old_sheet->net_labels.at(it.uuid))));
            old_sheet->net_labels.erase(it.uuid);
        } break;
        case ObjectType::BUS_LABEL: {
            new_sheet->bus_labels.insert(std::make_pair(it.uuid, std::move(old_sheet->bus_labels.at(it.uuid))));
            old_sheet->bus_labels.erase(it.uuid);
        } break;
        case ObjectType::BUS_RIPPER: {
            new_sheet->bus_rippers.insert(std::make_pair(it.uuid, std::move(old_sheet->bus_rippers.at(it.uuid))));
            old_sheet->bus_rippers.erase(it.uuid);
        } break;
        case ObjectType::JUNCTION: {
            new_sheet->junctions.insert(std::make_pair(it.uuid, std::move(old_sheet->junctions.at(it.uuid))));
            old_sheet->junctions.erase(it.uuid);
        } break;
        case ObjectType::POWER_SYMBOL: {
            new_sheet->power_symbols.insert(std::make_pair(it.uuid, std::move(old_sheet->power_symbols.at(it.uuid))));
            old_sheet->power_symbols.erase(it.uuid);
        } break;
        case ObjectType::LINE_NET: {
            new_sheet->net_lines.insert(std::make_pair(it.uuid, std::move(old_sheet->net_lines.at(it.uuid))));
            old_sheet->net_lines.erase(it.uuid);
        } break;
        case ObjectType::SCHEMATIC_SYMBOL: {
            new_sheet->symbols.insert(std::make_pair(it.uuid, std::move(old_sheet->symbols.at(it.uuid))));
            old_sheet->symbols.erase(it.uuid);
        } break;
        case ObjectType::SCHEMATIC_BLOCK_SYMBOL: {
            new_sheet->block_symbols.insert(std::make_pair(it.uuid, std::move(old_sheet->block_symbols.at(it.uuid))));
            old_sheet->block_symbols.erase(it.uuid);
        } break;
        case ObjectType::TEXT: {
            new_sheet->texts.insert(std::make_pair(it.uuid, std::move(old_sheet->texts.at(it.uuid))));
            old_sheet->texts.erase(it.uuid);
        } break;
        case ObjectType::LINE: {
            new_sheet->lines.insert(std::make_pair(it.uuid, std::move(old_sheet->lines.at(it.uuid))));
            old_sheet->lines.erase(it.uuid);
        } break;
        case ObjectType::SCHEMATIC_NET_TIE: {
            new_sheet->net_ties.insert(std::make_pair(it.uuid, std::move(old_sheet->net_ties.at(it.uuid))));
            old_sheet->net_ties.erase(it.uuid);
        } break;
        case ObjectType::ARC: {
            new_sheet->arcs.insert(std::make_pair(it.uuid, std::move(old_sheet->arcs.at(it.uuid))));
            old_sheet->arcs.erase(it.uuid);
        } break;

        default:;
        }
    }
    core_schematic.get_current_schematic()->update_refs();

    core_schematic.set_needs_save();
    core_schematic.rebuild("move to other sheet");
    canvas_update();
    canvas->set_selection(selection);
    tool_begin(ToolID::MOVE);
}

ActionToolID ImpSchematic::get_doubleclick_action(ObjectType type, const UUID &uu)
{
    const auto a = ImpBase::get_doubleclick_action(type, uu);
    if (a.is_valid())
        return a;

    switch (type) {
    case ObjectType::NET_LABEL:
    case ObjectType::LINE_NET:
        return ToolID::ENTER_DATUM;

    case ObjectType::SCHEMATIC_BLOCK_SYMBOL:
        return ActionID::PUSH_INTO_BLOCK;

    default:
        return {};
    }
}

void ImpSchematic::update_unplaced()
{
    if (core_schematic.get_block_symbol_mode()) {
        std::map<UUIDPath<2>, std::string> ports;
        const auto &block = *core_schematic.get_current_block();
        for (const auto &[uu, net] : block.nets) {
            if (net.is_port)
                ports.emplace(std::piecewise_construct, std::forward_as_tuple(uu),
                              std::forward_as_tuple(block.get_net_name(uu)));
        }

        for (auto &[uu, port] : core_schematic.get_block_symbol().ports) {
            ports.erase(port.net);
        }
        unplaced_box->update(ports);
    }
    else {
        unplaced_box->update(core_schematic.get_current_schematic()->get_unplaced_gates());
    }
}

ToolID ImpSchematic::get_tool_for_drag_move(bool ctrl, const std::set<SelectableRef> &sel) const
{
    if (preferences.schematic.bend_non_ortho && sel.size() == 1 && sel.begin()->type == ObjectType::LINE_NET) {
        const auto &ln = core_schematic.get_sheet()->net_lines.at(sel.begin()->uuid);
        auto from = ln.from.get_position();
        auto to = ln.to.get_position();
        if (!((from.x == to.x) || (from.y == to.y))) {
            return ToolID::BEND_LINE_NET;
        }
    }
    return ImpBase::get_tool_for_drag_move(ctrl, sel);
}

void ImpSchematic::handle_extra_button(const GdkEventButton *button_event)
{
    if (!preferences.mouse.switch_sheets)
        return;

    switch (button_event->button) {
    case 6:
    case 8:
        trigger_action(ActionID::NEXT_SHEET);
        break;

    case 7:
    case 9:
        trigger_action(ActionID::PREV_SHEET);
        break;

    default:;
    }
}

void ImpSchematic::expand_selection_for_property_panel(std::set<SelectableRef> &sel_extra,
                                                       const std::set<SelectableRef> &sel)
{
    if (core_schematic.get_block_symbol_mode())
        return;
    const auto &sheet = *core_schematic.get_sheet();
    for (const auto &it : sel) {
        switch (it.type) {
        case ObjectType::SCHEMATIC_SYMBOL:
            sel_extra.emplace(core_schematic.get_sheet()->symbols.at(it.uuid).component->uuid, ObjectType::COMPONENT);
            break;
        case ObjectType::SCHEMATIC_BLOCK_SYMBOL:
            sel_extra.emplace(core_schematic.get_sheet()->block_symbols.at(it.uuid).block_instance->uuid,
                              ObjectType::BLOCK_INSTANCE);
            break;
        case ObjectType::JUNCTION:
            if (sheet.junctions.at(it.uuid).net) {
                sel_extra.emplace(sheet.junctions.at(it.uuid).net->uuid, ObjectType::NET);
            }
            break;
        case ObjectType::LINE_NET: {
            LineNet &li = core_schematic.get_sheet()->net_lines.at(it.uuid);
            if (li.net) {
                sel_extra.emplace(li.net->uuid, ObjectType::NET);
            }
        } break;
        case ObjectType::NET_LABEL: {
            NetLabel &la = core_schematic.get_sheet()->net_labels.at(it.uuid);
            if (la.junction->net) {
                sel_extra.emplace(la.junction->net->uuid, ObjectType::NET);
            }
        } break;
        case ObjectType::POWER_SYMBOL: {
            PowerSymbol &sym = core_schematic.get_sheet()->power_symbols.at(it.uuid);
            if (sym.net) {
                sel_extra.emplace(sym.net->uuid, ObjectType::NET);
            }
        } break;
        default:;
        }
    }
}

void ImpSchematic::update_monitor()
{
    ItemSet mon_items;
    for (const auto &[uu, block] : core_schematic.get_blocks().blocks) {
        {
            ItemSet items = block.block.get_pool_items_used();
            mon_items.insert(items.begin(), items.end());
        }
        {
            ItemSet items = block.schematic.get_pool_items_used();
            mon_items.insert(items.begin(), items.end());
        }
    }
    set_monitor_items(mon_items);
}

} // namespace horizon
