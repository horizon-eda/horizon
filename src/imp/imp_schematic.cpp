#include "imp_schematic.hpp"
#include "canvas/canvas_gl.hpp"
#include "export_pdf/export_pdf.hpp"
#include "pool/part.hpp"
#include "rules/rules_window.hpp"
#include "widgets/sheet_box.hpp"
#include "util/gtk_util.hpp"
#include "util/util.hpp"
#include "util/str_util.hpp"
#include "hud_util.hpp"
#include "bom_export_window.hpp"
#include "pdf_export_window.hpp"
#include "nlohmann/json.hpp"

namespace horizon {
ImpSchematic::ImpSchematic(const std::string &schematic_filename, const std::string &block_filename,
                           const PoolParams &pool_params)
    : ImpBase(pool_params), core_schematic(schematic_filename, block_filename, *pool),
      project_dir(Glib::path_get_dirname(schematic_filename))
{
    core = &core_schematic;
    core_schematic.signal_tool_changed().connect(sigc::mem_fun(*this, &ImpSchematic::handle_tool_change));
    core_schematic.signal_rebuilt().connect(sigc::mem_fun(*this, &ImpSchematic::handle_core_rebuilt));
}

void ImpSchematic::canvas_update()
{
    canvas->update(*core_schematic.get_canvas_data());
    warnings_box->update(core_schematic.get_sheet()->warnings);
}

void ImpSchematic::handle_select_sheet(Sheet *sh)
{
    if (sh == core_schematic.get_sheet())
        return;

    auto v = canvas->get_scale_and_offset();
    sheet_views[core_schematic.get_sheet()->uuid] = v;
    core_schematic.set_sheet(sh->uuid);
    canvas_update();
    if (sheet_views.count(sh->uuid)) {
        auto v2 = sheet_views.at(sh->uuid);
        canvas->set_scale_and_offset(v2.first, v2.second);
    }
    update_highlights();
}

void ImpSchematic::handle_remove_sheet(Sheet *sh)
{
    core_schematic.delete_sheet(sh->uuid);
    canvas_update();
}

void ImpSchematic::update_highlights()
{
    std::map<UUID, std::set<ObjectRef>> highlights_for_sheet;
    auto sch = core_schematic.get_schematic();
    for (const auto &it_sheet : sch->sheets) {
        auto sheet = &it_sheet.second;
        for (const auto &it : highlights) {
            if (it.type == ObjectType::NET) {
                for (const auto &it_line : sheet->net_lines) {
                    if (it_line.second.net.uuid == it.uuid) {
                        highlights_for_sheet[sheet->uuid].emplace(ObjectType::LINE_NET, it_line.first);
                    }
                }
                for (const auto &it_junc : sheet->junctions) {
                    if (it_junc.second.net.uuid == it.uuid) {
                        highlights_for_sheet[sheet->uuid].emplace(ObjectType::JUNCTION, it_junc.first);
                    }
                }
                for (const auto &it_label : sheet->net_labels) {
                    if (it_label.second.junction->net.uuid == it.uuid) {
                        highlights_for_sheet[sheet->uuid].emplace(ObjectType::NET_LABEL, it_label.first);
                    }
                }
                for (const auto &it_sym : sheet->power_symbols) {
                    if (it_sym.second.junction->net.uuid == it.uuid) {
                        highlights_for_sheet[sheet->uuid].emplace(ObjectType::POWER_SYMBOL, it_sym.first);
                    }
                }
                for (const auto &it_sym : sheet->symbols) {
                    auto component = it_sym.second.component;
                    for (const auto &it_pin : it_sym.second.symbol.pins) {
                        UUIDPath<2> connpath(it_sym.second.gate->uuid, it_pin.second.uuid);
                        if (component->connections.count(connpath)) {
                            auto net = component->connections.at(connpath).net;
                            if (net.uuid == it.uuid) {
                                highlights_for_sheet[sheet->uuid].emplace(ObjectType::SYMBOL_PIN, it_pin.first,
                                                                          it_sym.first);
                            }
                        }
                    }
                }
            }
            else if (it.type == ObjectType::COMPONENT) {
                for (const auto &it_sym : sheet->symbols) {
                    auto component = it_sym.second.component;
                    if (component->uuid == it.uuid) {
                        highlights_for_sheet[sheet->uuid].emplace(ObjectType::SCHEMATIC_SYMBOL, it_sym.first);
                    }
                }
            }
        }
        sheet_box->update_highlights(sheet->uuid, highlights_for_sheet[sheet->uuid].size());
    }

    auto sheet = core_schematic.get_sheet();
    canvas->set_flags_all(0, Triangle::FLAG_HIGHLIGHT);
    canvas->set_highlight_enabled(highlights_for_sheet[sheet->uuid].size());
    for (const auto &it : highlights_for_sheet[sheet->uuid]) {
        canvas->set_flags(it, Triangle::FLAG_HIGHLIGHT, 0);
    }
}

bool ImpSchematic::handle_broadcast(const json &j)
{
    if (!ImpBase::handle_broadcast(j)) {
        if (core_schematic.tool_is_active())
            return true;
        std::string op = j.at("op");
        guint32 timestamp = j.value("time", 0);
        if (op == "place-part") {
            main_window->present(timestamp);
            part_from_project_manager = j.at("part").get<std::string>();
            tool_begin(ToolID::ADD_PART);
        }
        if (op == "assign-part") {
            main_window->present(timestamp);
            part_from_project_manager = j.at("part").get<std::string>();
            tool_begin(ToolID::ASSIGN_PART);
        }
        else if (op == "highlight" && cross_probing_enabled) {
            if (!core_schematic.tool_is_active()) {
                highlights.clear();
                const json &o = j["objects"];
                for (auto it = o.cbegin(); it != o.cend(); ++it) {
                    auto type = static_cast<ObjectType>(it.value().at("type").get<int>());
                    UUID uu(it.value().at("uuid").get<std::string>());
                    highlights.emplace(type, uu);
                }
                update_highlights();
            }
        }
    }
    return true;
}

void ImpSchematic::handle_selection_cross_probe()
{
    if (core_schematic.tool_is_active())
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
            uu = sym.component->uuid;
        } break;
        default:;
        }

        if (type != ObjectType::INVALID) {
            k["type"] = static_cast<int>(type);
            k["uuid"] = (std::string)uu;
            j["selection"].push_back(k);
        }
    }
    send_json(j);
}

int ImpSchematic::get_board_pid()
{
    json j;
    j["op"] = "get-board-pid";
    return this->send_json(j);
}

void ImpSchematic::construct()
{

    main_window->set_title("Schematic - Interactive Manipulator");
    state_store = std::make_unique<WindowStateStore>(main_window, "imp-schematic");

    sheet_box = Gtk::manage(new SheetBox(&core_schematic));
    sheet_box->show_all();
    sheet_box->signal_add_sheet().connect([this] {
        core_schematic.add_sheet();
        std::cout << "add sheet" << std::endl;
    });
    sheet_box->signal_remove_sheet().connect(sigc::mem_fun(*this, &ImpSchematic::handle_remove_sheet));
    sheet_box->signal_select_sheet().connect(sigc::mem_fun(*this, &ImpSchematic::handle_select_sheet));
    main_window->left_panel->pack_start(*sheet_box, false, false, 0);

    hamburger_menu->append("Annotate", "win.annotate");
    add_tool_action(ToolID::ANNOTATE, "annotate");

    hamburger_menu->append("Buses...", "win.manage_buses");
    add_tool_action(ToolID::MANAGE_BUSES, "manage_buses");

    hamburger_menu->append("Net classes...", "win.manage_nc");
    add_tool_action(ToolID::MANAGE_NET_CLASSES, "manage_nc");

    hamburger_menu->append("Power Nets...", "win.manage_pn");
    add_tool_action(ToolID::MANAGE_POWER_NETS, "manage_pn");

    hamburger_menu->append("Schematic properties", "win.sch_properties");
    add_tool_action(ToolID::EDIT_SCHEMATIC_PROPERTIES, "sch_properties");

    hamburger_menu->append("BOM Export", "win.bom_export");
    main_window->add_action("bom_export", [this] { trigger_action(ActionID::BOM_EXPORT_WINDOW); });

    hamburger_menu->append("PDF Export", "win.export_pdf");

    main_window->add_action("export_pdf", [this] { trigger_action(ActionID::PDF_EXPORT_WINDOW); });

    if (sockets_connected) {
        canvas->signal_selection_changed().connect(sigc::mem_fun(*this, &ImpSchematic::handle_selection_cross_probe));
        hamburger_menu->append("Cross probing", "win.cross_probing");
        auto cp_action = main_window->add_action_bool("cross_probing", true);
        cross_probing_enabled = true;
        cp_action->signal_change_state().connect([this, cp_action](const Glib::VariantBase &v) {
            cross_probing_enabled = Glib::VariantBase::cast_dynamic<Glib::Variant<bool>>(v).get();
            g_simple_action_set_state(cp_action->gobj(), g_variant_new_boolean(cross_probing_enabled));
            if (!cross_probing_enabled && !core_schematic.tool_is_active()) {
                highlights.clear();
                update_highlights();
            }
        });
    }

    canvas->set_highlight_mode(CanvasGL::HighlightMode::DIM);

    {
        auto button = Gtk::manage(new Gtk::Button("Place part"));
        button->signal_clicked().connect([this] { trigger_action(ActionID::PLACE_PART); });
        button->show();
        core.r->signal_tool_changed().connect([button](ToolID t) { button->set_sensitive(t == ToolID::NONE); });
        main_window->header->pack_end(*button);
    }

    connect_action(ActionID::PLACE_PART, [this](const auto &conn) {
        if (!sockets_connected) {
            this->tool_begin(ToolID::ADD_PART);
        }
        else {
            auto ev = gtk_get_current_event();
            json j;
            j["op"] = "show-browser";
            j["time"] = gdk_event_get_time(ev);
            allow_set_foreground_window(mgr_pid);
            this->send_json(j);
        }
    });

    if (sockets_connected) {
        connect_action(ActionID::TO_BOARD, [this](const auto &conn) {
            auto ev = gtk_get_current_event();
            json j;
            j["op"] = "to-board";
            j["time"] = gdk_event_get_time(ev);
            j["selection"] = nullptr;
            for (const auto &it : canvas->get_selection()) {
                auto sheet = core_schematic.get_sheet();
                if (it.type == ObjectType::SCHEMATIC_SYMBOL) {
                    auto &sym = sheet->symbols.at(it.uuid);
                    json k;
                    k["type"] = static_cast<int>(ObjectType::COMPONENT);
                    k["uuid"] = (std::string)sym.component->uuid;
                    j["selection"].push_back(k);
                }
            }
            canvas->set_selection({});
            allow_set_foreground_window(this->get_board_pid());
            this->send_json(j);
        });
        set_action_sensitive(make_action(ActionID::TO_BOARD), false);
        auto button = create_action_button(make_action(ActionID::TO_BOARD));
        button->set_tooltip_text("Place selected components on board");
        button->show();
        main_window->header->pack_end(*button);

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
            auto ev = gtk_get_current_event();
            json j;
            j["time"] = gdk_event_get_time(ev);
            j["op"] = "reload-netlist";
            this->send_json(j);
        });
        set_action_sensitive(make_action(ActionID::SAVE_RELOAD_NETLIST), false);

        connect_action(ActionID::GO_TO_BOARD, [this](const auto &conn) {
            auto ev = gtk_get_current_event();
            json j;
            j["time"] = gdk_event_get_time(ev);
            j["op"] = "present-board";
            allow_set_foreground_window(this->get_board_pid());
            this->send_json(j);
        });
        set_action_sensitive(make_action(ActionID::GO_TO_BOARD), false);

        connect_action(ActionID::SHOW_IN_POOL_MANAGER, [this](const auto &conn) {
            for (const auto &it : canvas->get_selection()) {
                auto sheet = core_schematic.get_sheet();
                if (it.type == ObjectType::SCHEMATIC_SYMBOL) {
                    auto &sym = sheet->symbols.at(it.uuid);
                    if (sym.component->part) {
                        json j;
                        j["op"] = "show-in-pool-mgr";
                        j["type"] = "part";
                        UUID pool_uuid;
                        pool->get_filename(ObjectType::PART, sym.component->part->uuid, &pool_uuid);
                        j["pool_uuid"] = (std::string)pool_uuid;
                        j["uuid"] = (std::string)sym.component->part->uuid;
                        this->send_json(j);
                        break;
                    }
                }
            }
        });
        set_action_sensitive(make_action(ActionID::SHOW_IN_POOL_MANAGER), false);
    }

    connect_action(ActionID::MOVE_TO_OTHER_SHEET,
                   std::bind(&ImpSchematic::handle_move_to_other_sheet, this, std::placeholders::_1));


    connect_action(ActionID::HIGHLIGHT_NET, [this](const auto &a) {
        highlights.clear();
        auto sheet = core_schematic.get_sheet();
        for (const auto &it : canvas->get_selection()) {
            switch (it.type) {
            case ObjectType::LINE_NET: {
                auto &li = sheet->net_lines.at(it.uuid);
                if (li.net) {
                    highlights.emplace(ObjectType::NET, li.net->uuid);
                }
            } break;
            case ObjectType::NET_LABEL: {
                auto &la = sheet->net_labels.at(it.uuid);
                if (la.junction->net) {
                    highlights.emplace(ObjectType::NET, la.junction->net->uuid);
                }
            } break;
            case ObjectType::POWER_SYMBOL: {
                auto &sym = sheet->power_symbols.at(it.uuid);
                if (sym.junction->net) {
                    highlights.emplace(ObjectType::NET, sym.junction->net->uuid);
                }
            } break;
            case ObjectType::JUNCTION: {
                auto &ju = sheet->junctions.at(it.uuid);
                if (ju.net) {
                    highlights.emplace(ObjectType::NET, ju.net->uuid);
                }
            } break;
            default:;
            }
        }
        this->update_highlights();
    });

    connect_action(ActionID::HIGHLIGHT_GROUP, sigc::mem_fun(*this, &ImpSchematic::handle_highlight_group_tag));
    connect_action(ActionID::HIGHLIGHT_TAG, sigc::mem_fun(*this, &ImpSchematic::handle_highlight_group_tag));


    bom_export_window = BOMExportWindow::create(main_window, core_schematic.get_block(),
                                                core_schematic.get_bom_export_settings(), project_dir);

    connect_action(ActionID::BOM_EXPORT_WINDOW, [this](const auto &c) { bom_export_window->present(); });
    connect_action(ActionID::EXPORT_BOM, [this](const auto &c) {
        bom_export_window->present();
        bom_export_window->generate();
    });

    bom_export_window->signal_changed().connect([this] { core_schematic.set_needs_save(); });
    core.r->signal_tool_changed().connect([this](ToolID t) { bom_export_window->set_can_export(t == ToolID::NONE); });
    core.r->signal_rebuilt().connect([this] { bom_export_window->update_preview(); });


    pdf_export_window = PDFExportWindow::create(main_window, *core_schematic.get_schematic(),
                                                *core_schematic.get_pdf_export_settings(), project_dir);
    pdf_export_window->signal_changed().connect([this] { core_schematic.set_needs_save(); });
    connect_action(ActionID::PDF_EXPORT_WINDOW, [this](const auto &c) { pdf_export_window->present(); });
    connect_action(ActionID::EXPORT_PDF, [this](const auto &c) {
        pdf_export_window->present();
        pdf_export_window->generate();
    });


    grid_spin_button->set_sensitive(false);

    rules_window->signal_goto().connect([this](Coordi location, UUID sheet) {
        auto sch = core_schematic.get_schematic();
        if (sch->sheets.count(sheet)) {
            sheet_box->select_sheet(sheet);
            canvas->center_and_zoom(location);
        }
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
} // namespace horizon

void ImpSchematic::update_action_sensitivity()
{
    auto sel = canvas->get_selection();
    if (sockets_connected) {
        json req;
        req["op"] = "has-board";
        bool has_board = send_json(req);
        set_action_sensitive(make_action(ActionID::SAVE_RELOAD_NETLIST), has_board);
        set_action_sensitive(make_action(ActionID::GO_TO_BOARD), has_board);
        auto n_sym = std::count_if(sel.begin(), sel.end(),
                                   [](const auto &x) { return x.type == ObjectType::SCHEMATIC_SYMBOL; });
        set_action_sensitive(make_action(ActionID::SHOW_IN_BROWSER), n_sym == 1);
        set_action_sensitive(make_action(ActionID::SHOW_IN_POOL_MANAGER), n_sym == 1);
        if (!has_board) {
            set_action_sensitive(make_action(ActionID::TO_BOARD), false);
        }
        else {
            set_action_sensitive(make_action(ActionID::TO_BOARD), n_sym);
        }
    }
    else {
        set_action_sensitive(make_action(ActionID::TO_BOARD), false);
        set_action_sensitive(make_action(ActionID::SHOW_IN_BROWSER), false);
        set_action_sensitive(make_action(ActionID::SHOW_IN_POOL_MANAGER), false);
    }
    set_action_sensitive(make_action(ActionID::MOVE_TO_OTHER_SHEET), sel.size() > 0);

    set_action_sensitive(make_action(ActionID::HIGHLIGHT_NET), std::any_of(sel.begin(), sel.end(), [](const auto &x) {
                             switch (x.type) {
                             case ObjectType::LINE_NET:
                             case ObjectType::NET_LABEL:
                             case ObjectType::JUNCTION:
                             case ObjectType::POWER_SYMBOL:
                                 return true;

                             default:
                                 return false;
                             }
                         }));
    bool can_higlight_group = false;
    bool can_higlight_tag = false;
    if (sel.size() == 1 && (*sel.begin()).type == ObjectType::SCHEMATIC_SYMBOL) {
        auto uu = (*sel.begin()).uuid;
        if (core_schematic.get_sheet()->symbols.count(uu)) {
            auto comp = core_schematic.get_schematic_symbol(uu)->component;
            can_higlight_group = comp->group;
            can_higlight_tag = comp->tag;
        }
    }

    set_action_sensitive(make_action(ActionID::HIGHLIGHT_GROUP), can_higlight_group);
    set_action_sensitive(make_action(ActionID::HIGHLIGHT_TAG), can_higlight_tag);


    ImpBase::update_action_sensitivity();
}

std::string ImpSchematic::get_hud_text(std::set<SelectableRef> &sel)
{
    std::string s;
    if (sel_count_type(sel, ObjectType::SCHEMATIC_SYMBOL) == 1) {
        const auto &sym = core_schematic.get_sheet()->symbols.at(sel_find_one(sel, ObjectType::SCHEMATIC_SYMBOL));
        s += "<b>Symbol " + sym.component->refdes + "</b>\n";
        s += get_hud_text_for_part(sym.component->part);
        sel_erase_type(sel, ObjectType::SCHEMATIC_SYMBOL);
    }
    trim(s);
    return s;
}

void ImpSchematic::handle_export_pdf()
{
    pdf_export_window->present();
}

void ImpSchematic::handle_core_rebuilt()
{
    sheet_box->update();
}

void ImpSchematic::handle_tool_change(ToolID id)
{
    ImpBase::handle_tool_change(id);
    sheet_box->set_sensitive(id == ToolID::NONE);
}

void ImpSchematic::handle_highlight_group_tag(const ActionConnection &a)
{
    bool tag_mode = a.action_id == ActionID::HIGHLIGHT_TAG;
    auto sel = canvas->get_selection();
    if (sel.size() == 1 && (*sel.begin()).type == ObjectType::SCHEMATIC_SYMBOL) {
        auto comp = core_schematic.get_schematic_symbol((*sel.begin()).uuid)->component;
        UUID uu = tag_mode ? comp->tag : comp->group;
        if (!uu)
            return;
        highlights.clear();
        for (const auto &it_sheet : core_schematic.get_schematic()->sheets) {
            for (const auto &it_sym : it_sheet.second.symbols) {
                if ((tag_mode && (it_sym.second.component->tag == uu))
                    || (!tag_mode && (it_sym.second.component->group == uu))) {
                    highlights.emplace(ObjectType::COMPONENT, it_sym.second.component->uuid);
                }
            }
        }
        this->update_highlights();
    }
}

void ImpSchematic::handle_maybe_drag()
{
    if (!preferences.schematic.drag_start_net_line) {
        ImpBase::handle_maybe_drag();
        return;
    }

    auto target = canvas->get_current_target();
    if (target.type == ObjectType::SYMBOL_PIN || target.type == ObjectType::JUNCTION) {
        std::cout << "click pin" << std::endl;
        canvas->inhibit_drag_selection();
        target_drag_begin = target;
        cursor_pos_drag_begin = canvas->get_cursor_pos_win();
    }
    else {
        ImpBase::handle_maybe_drag();
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
        const auto sym = core_schematic.get_schematic_symbol(target_drag_begin.path.at(0));
        const auto &pin = sym->symbol.pins.at(target_drag_begin.path.at(1));
        auto orientation = pin.get_orientation_for_placement(sym->placement);
        start = drag_does_start(delta, orientation);
    }
    else if (target_drag_begin.type == ObjectType::JUNCTION) {
        start = delta.mag_sq() > (50 * 50);
    }

    if (start) {
        {
            highlights.clear();
            update_highlights();
            ToolArgs args;
            args.coords = target_drag_begin.p;
            ToolResponse r = core.r->tool_begin(ToolID::DRAW_NET, args, imp_interface.get());
            tool_process(r);
        }
        {
            ToolArgs args;
            args.type = ToolEventType::CLICK;
            args.coords = target_drag_begin.p;
            args.button = 1;
            args.target = target_drag_begin;
            args.work_layer = canvas->property_work_layer();
            ToolResponse r = core.r->tool_update(args);
            tool_process(r);
        }
        target_drag_begin = Target();
    }
}

void ImpSchematic::search_center(const Core::SearchResult &res)
{
    if (res.sheet != core_schematic.get_sheet()->uuid) {
        sheet_box->select_sheet(res.sheet);
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

    std::vector<const Sheet *> sheets;
    for (const auto &it : sch->sheets) {
        sheets.push_back(&it.second);
    }
    std::sort(sheets.begin(), sheets.end(), [](auto a, auto b) { return a->index < b->index; });

    for (const auto it : sheets) {
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
                auto &la = core.c->get_sheet()->net_labels.at(it.uuid);
                new_sel.emplace(la.junction->uuid, ObjectType::JUNCTION);
            } break;
            case ObjectType::BUS_LABEL: {
                auto &la = core.c->get_sheet()->bus_labels.at(it.uuid);
                new_sel.emplace(la.junction->uuid, ObjectType::JUNCTION);
            } break;
            case ObjectType::POWER_SYMBOL: {
                auto &ps = core.c->get_sheet()->power_symbols.at(it.uuid);
                new_sel.emplace(ps.junction->uuid, ObjectType::JUNCTION);
            } break;
            case ObjectType::BUS_RIPPER: {
                auto &rip = core.c->get_sheet()->bus_rippers.at(it.uuid);
                new_sel.emplace(rip.junction->uuid, ObjectType::JUNCTION);
            } break;

            case ObjectType::LINE_NET: {
                auto line = &core.c->get_sheet()->net_lines.at(it.uuid);
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
                }
            } break;

            case ObjectType::SCHEMATIC_SYMBOL: {
                auto sym = core.c->get_schematic_symbol(it.uuid);
                for (const auto &itt : sym->texts) {
                    new_sel.emplace(itt->uuid, ObjectType::TEXT);
                }
            } break;

            default:;
            }
        }

        // other direction
        for (const auto &it : core.c->get_sheet()->net_labels) {
            if (selection.count(SelectableRef(it.second.junction->uuid, ObjectType::JUNCTION))) {
                new_sel.emplace(it.first, ObjectType::NET_LABEL);
            }
        }
        for (const auto &it : core.c->get_sheet()->bus_labels) {
            if (selection.count(SelectableRef(it.second.junction->uuid, ObjectType::JUNCTION))) {
                new_sel.emplace(it.first, ObjectType::BUS_LABEL);
            }
        }
        for (const auto &it : core.c->get_sheet()->bus_rippers) {
            if (selection.count(SelectableRef(it.second.junction->uuid, ObjectType::JUNCTION))) {
                new_sel.emplace(it.first, ObjectType::BUS_RIPPER);
            }
        }
        for (const auto &it : core.c->get_sheet()->power_symbols) {
            if (selection.count(SelectableRef(it.second.junction->uuid, ObjectType::JUNCTION))) {
                new_sel.emplace(it.first, ObjectType::POWER_SYMBOL);
            }
        }
        for (const auto &it : core.c->get_sheet()->net_lines) {
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
            }
            if (add_line) {
                new_sel.emplace(it.first, ObjectType::LINE_NET);
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

    auto old_sheet = core.c->get_sheet();
    Sheet *new_sheet = nullptr;
    {
        SelectSheetDialog dia(core.c->get_schematic(), old_sheet);
        dia.set_transient_for(*main_window);
        if (dia.run() == Gtk::RESPONSE_OK) {
            new_sheet = &core.c->get_schematic()->sheets.at(dia.selected_sheet);
        }
    }
    if (!new_sheet)
        return;
    sheet_box->select_sheet(new_sheet->uuid);
    assert(core.c->get_sheet() == new_sheet);

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
        case ObjectType::TEXT: {
            new_sheet->texts.insert(std::make_pair(it.uuid, std::move(old_sheet->texts.at(it.uuid))));
            old_sheet->texts.erase(it.uuid);
        } break;

        default:;
        }
    }
    core.c->get_schematic()->update_refs();

    core.c->commit();
    core.c->rebuild();
    canvas_update();
    canvas->set_selection(selection);
    tool_begin(ToolID::MOVE);
}

std::pair<ActionID, ToolID> ImpSchematic::get_doubleclick_action(ObjectType type, const UUID &uu)
{
    auto a = ImpBase::get_doubleclick_action(type, uu);
    if (a.first != ActionID::NONE)
        return a;
    switch (type) {
    case ObjectType::NET_LABEL:
    case ObjectType::LINE_NET:
        return make_action(ToolID::ENTER_DATUM);
        break;
    default:
        return {ActionID::NONE, ToolID::NONE};
    }
}

} // namespace horizon
