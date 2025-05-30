#include "imp_board.hpp"
#include "3d/3d_view.hpp"
#include "canvas/canvas_gl.hpp"
#include "canvas/canvas_pads.hpp"
#include "fab_output_window.hpp"
#include "step_export_window.hpp"
#include "pool/part.hpp"
#include "rules/rules_window.hpp"
#include "widgets/board_display_options.hpp"
#include "widgets/layer_box.hpp"
#include "tuning_window.hpp"
#include "util/selection_util.hpp"
#include "util/util.hpp"
#include "util/geom_util.hpp"
#include "util/str_util.hpp"
#include "canvas/annotation.hpp"
#include "export_pdf/export_pdf_board.hpp"
#include "board/board_layers.hpp"
#include "pdf_export_window.hpp"
#include "widgets/unplaced_box.hpp"
#include "pnp_export_window.hpp"
#include "airwire_filter_window.hpp"
#include "core/tool_id.hpp"
#include "widgets/action_button.hpp"
#include "parts_window.hpp"
#include "actions.hpp"
#include "in_tool_action.hpp"
#include "util/gtk_util.hpp"

namespace horizon {
ImpBoard::ImpBoard(const CoreBoard::Filenames &filenames, const PoolParams &pool_params)
    : ImpLayer(pool_params), core_board(filenames, *pool, *pool_caching),
      project_dir(Glib::path_get_dirname(filenames.board)), searcher(core_board)
{
    core = &core_board;
    core_board.signal_tool_changed().connect(sigc::mem_fun(*this, &ImpBase::handle_tool_change));
    load_meta();
}

void ImpBoard::canvas_update()
{
    canvas->update(core_board.get_canvas_data());
    warnings_box->update(core_board.get_board()->warnings);
    apply_net_colors();
    update_highlights();
    tuning_window->update();
    update_text_owner_annotation();
}

void ImpBoard::update_airwire_annotation()
{
    airwire_annotation->clear();
    const auto work_layer = canvas->property_work_layer().get_value();
    const auto work_layer_only = layer_box->property_layer_mode() != CanvasGL::LayerMode::AS_IS;
    for (const auto &[net, airwires] : core_board.get_board()->airwires) {
        if (!airwire_filter_window->airwire_is_visible(net))
            continue;
        const bool highlight = highlights.count(ObjectRef(ObjectType::NET, net));
        uint8_t color2 = 0;
        if (net_color_map.count(net))
            color2 = net_color_map.at(net);
        if (core_board.tool_is_active() && !highlight && highlights.size())
            continue;
        for (const auto &airwire : airwires) {
            const auto layers = airwire.get_layers();
            if (work_layer_only && !layers.overlaps(work_layer))
                continue;
            if (!canvas->layer_is_visible(layers))
                continue;

            airwire_annotation->draw_line(airwire.from.get_position(), airwire.to.get_position(), ColorP::AIRWIRE, 0,
                                          highlight, color2);
        }
    }
}

void ImpBoard::update_highlights()
{
    canvas->set_flags_all(0, TriangleInfo::FLAG_HIGHLIGHT);
    canvas->set_highlight_enabled(highlights.size());
    for (const auto &it : highlights) {
        if (it.type == ObjectType::NET) {
            for (const auto &it_track : core_board.get_board()->tracks) {
                if (it_track.second.net.uuid == it.uuid) {
                    ObjectRef ref(ObjectType::TRACK, it_track.first);
                    canvas->set_flags(ref, TriangleInfo::FLAG_HIGHLIGHT, 0);
                }
            }
            for (const auto &it_plane : core_board.get_board()->planes) {
                if (it_plane.second.net.uuid == it.uuid) {
                    ObjectRef ref(ObjectType::PLANE, it_plane.first);
                    canvas->set_flags(ref, TriangleInfo::FLAG_HIGHLIGHT, 0);
                }
            }
            for (const auto &it_via : core_board.get_board()->vias) {
                if (it_via.second.junction->net.uuid == it.uuid) {
                    ObjectRef ref(ObjectType::VIA, it_via.first);
                    canvas->set_flags(ref, TriangleInfo::FLAG_HIGHLIGHT, 0);
                }
            }
            for (const auto &it_pkg : core_board.get_board()->packages) {
                for (const auto &it_pad : it_pkg.second.package.pads) {
                    if (it_pad.second.net.uuid == it.uuid) {
                        ObjectRef ref(ObjectType::PAD, it_pad.first, it_pkg.first);
                        canvas->set_flags(ref, TriangleInfo::FLAG_HIGHLIGHT, 0);
                    }
                }
            }
        }
        if (it.type == ObjectType::COMPONENT) {
            for (const auto &it_pkg : core_board.get_board()->packages) {
                if (it_pkg.second.component->uuid == it.uuid) {
                    {
                        ObjectRef ref(ObjectType::BOARD_PACKAGE, it_pkg.first);
                        canvas->set_flags(ref, TriangleInfo::FLAG_HIGHLIGHT, 0);
                    }
                    for (const auto text : it_pkg.second.texts) {
                        ObjectRef ref(ObjectType::TEXT, text->uuid);
                        canvas->set_flags(ref, TriangleInfo::FLAG_HIGHLIGHT, 0);
                    }
                }
            }
        }
        else {
            canvas->set_flags(it, TriangleInfo::FLAG_HIGHLIGHT, 0);
        }
    }
    update_airwire_annotation();
}

void ImpBoard::apply_net_colors()
{
    canvas->reset_color2();
    std::map<UUID, std::vector<ObjectRef>> net_to_object_ref;
    for (const auto &[uu, track] : core_board.get_board()->tracks) {
        net_to_object_ref[track.net.uuid].emplace_back(ObjectType::TRACK, uu);
    }
    for (const auto &[uu, via] : core_board.get_board()->vias) {
        net_to_object_ref[via.junction->net.uuid].emplace_back(ObjectType::VIA, uu);
    }
    for (const auto &[uu_pkg, pkg] : core_board.get_board()->packages) {
        for (const auto &[uu_pad, pad] : pkg.package.pads) {
            net_to_object_ref[pad.net.uuid].emplace_back(ObjectType::PAD, uu_pad, uu_pkg);
        }
    }
    for (const auto &[uu, plane] : core_board.get_board()->planes) {
        net_to_object_ref[plane.net.uuid].emplace_back(ObjectType::PLANE, uu);
    }
    for (const auto &[net, color] : net_color_map) {
        for (const auto &ref : net_to_object_ref[net]) {
            canvas->set_color2(ref, color);
        }
    }
}

bool ImpBoard::handle_broadcast(const json &j)
{
    if (!ImpBase::handle_broadcast(j)) {
        const auto op = j.at("op").get<std::string>();
        guint32 timestamp = j.value("time", 0);
        std::string token = j.value("token", "");
        if (op == "highlight" && cross_probing_enabled && !core->tool_is_active()) {
            highlights.clear();
            const json &o = j["objects"];
            for (auto it = o.cbegin(); it != o.cend(); ++it) {
                auto type = static_cast<ObjectType>(it.value().at("type").get<int>());
                if (type == ObjectType::NET) {
                    const auto path = uuid_vec_from_string(it.value().at("uuid").get<std::string>());
                    for (const auto &[uu, net] : core_board.get_top_block()->nets) {
                        if (std::any_of(net.hrefs.begin(), net.hrefs.end(),
                                        [&path](const auto &x) { return x == path; })) {
                            highlights.emplace(type, uu);
                            break;
                        }
                    }
                }
                else {
                    UUID uu(it.value().at("uuid").get<std::string>());
                    highlights.emplace(type, uu);
                }
            }
            update_highlights();
        }
        else if (op == "place") {
            activate_window(main_window, timestamp, token);
            if (force_end_tool()) {
                std::set<SelectableRef> components;
                const json &o = j["components"];
                for (auto it = o.cbegin(); it != o.cend(); ++it) {
                    auto type = static_cast<ObjectType>(it.value().at("type").get<int>());
                    if (type == ObjectType::COMPONENT) {
                        UUID uu(it.value().at("uuid").get<std::string>());
                        components.emplace(uu, type);
                    }
                }
                tool_begin(ToolID::MAP_PACKAGE, true, components);
            }
        }
        else if (op == "reload-netlist") {
            activate_window(main_window, timestamp, token);
            if (force_end_tool())
                trigger_action(ActionID::RELOAD_NETLIST);
        }
        else if (op == "reload-netlist-hint" && !core->tool_is_active()) {
            reload_netlist_delay_conn = Glib::signal_timeout().connect(
                    [this] {
#if GTK_CHECK_VERSION(3, 22, 0)
                        reload_netlist_popover->popup();
#else
                        reload_netlist_popover->show();
#endif
                        return false;
                    },
                    500);
        }
        else if (op == "pool-updated") {
            return handle_pool_cache_update(j);
        }
    }
    return true;
}

void ImpBoard::handle_selection_cross_probe()
{
    json j;
    j["op"] = "board-select";
    j["selection"] = nullptr;
    std::set<UUID> pkgs;
    for (const auto &it : canvas->get_selection()) {
        json k;
        ObjectType type = ObjectType::INVALID;
        UUID uu;
        auto board = core_board.get_board();
        switch (it.type) {
        case ObjectType::TRACK: {
            auto &track = board->tracks.at(it.uuid);
            if (track.net) {
                type = ObjectType::NET;
                uu = track.net->uuid;
            }
        } break;
        case ObjectType::VIA: {
            auto &via = board->vias.at(it.uuid);
            if (via.junction->net) {
                type = ObjectType::NET;
                uu = via.junction->net->uuid;
            }
        } break;
        case ObjectType::JUNCTION: {
            auto &ju = board->junctions.at(it.uuid);
            if (ju.net) {
                type = ObjectType::NET;
                uu = ju.net->uuid;
            }
        } break;
        case ObjectType::BOARD_PACKAGE: {
            auto &pkg = board->packages.at(it.uuid);
            type = ObjectType::COMPONENT;
            uu = pkg.component->uuid;
            pkgs.insert(pkg.uuid);
        } break;
        default:;
        }

        if (type != ObjectType::INVALID) {
            k["type"] = static_cast<int>(type);
            if (type == ObjectType::COMPONENT) {
                k["uuid"] = (std::string)uu;
            }
            else if (type == ObjectType::NET) {
                const auto &net = board->block->nets.at(uu);
                std::vector<std::string> hrefs;
                for (const auto &path : net.hrefs) {
                    hrefs.push_back(uuid_vec_to_string(path));
                }
                k["uuid"] = hrefs;
            }
            j["selection"].push_back(k);
        }
    }
    view_3d_window->set_highlights(pkgs);
    send_json(j);
}

void transform_path(ClipperLib::Path &path, const Placement &tr)
{
    for (auto &it : path) {
        Coordi p(it.X, it.Y);
        auto q = tr.transform(p);
        it.X = q.x;
        it.Y = q.y;
    }
}

void ImpBoard::update_action_sensitivity()
{
    auto sel = canvas->get_selection();
    bool have_tracks = std::any_of(sel.begin(), sel.end(), [](const auto &x) { return x.type == ObjectType::TRACK; });
    int n_pkgs =
            std::count_if(sel.begin(), sel.end(), [](const auto &x) { return x.type == ObjectType::BOARD_PACKAGE; });
    set_action_sensitive(ActionID::TUNING_ADD_TRACKS, have_tracks);
    set_action_sensitive(ActionID::TUNING_ADD_TRACKS_ALL, have_tracks);
    {
        const auto x = sel_find_exactly_one(sel, ObjectType::BOARD_PACKAGE);
        bool sensitive = false;
        if (x && core_board.get_board()->packages.count(x->uuid)) {
            auto part = core_board.get_board()->packages.at(x->uuid).component->part;
            sensitive = part && part->get_datasheet().size();
        }
        set_action_sensitive(ActionID::OPEN_DATASHEET, sensitive);
    }

    const bool can_select_more = std::any_of(sel.begin(), sel.end(), [](const auto &x) {
        switch (x.type) {
        case ObjectType::TRACK:
        case ObjectType::JUNCTION:
        case ObjectType::VIA:
            return true;

        default:
            return false;
        }
    });

    const bool has_plane = std::any_of(sel.begin(), sel.end(), [this](const auto &x) {
        switch (x.type) {
        case ObjectType::POLYGON:
        case ObjectType::POLYGON_ARC_CENTER:
        case ObjectType::POLYGON_VERTEX:
        case ObjectType::POLYGON_EDGE: {
            if (!core_board.get_board()->polygons.count(x.uuid))
                return false;
            const auto &poly = *core_board.get_polygon(x.uuid);
            if (auto plane = dynamic_cast<const Plane *>(poly.usage.ptr))
                return true;
        } break;

        default:
            return false;
        }
        return false;
    });

    set_action_sensitive(ActionID::HIGHLIGHT_NET, can_select_more || has_plane);
    set_action_sensitive(ActionID::HIGHLIGHT_NET_CLASS, can_select_more || has_plane);
    set_action_sensitive(ActionID::SELECT_MORE, can_select_more);
    set_action_sensitive(ActionID::SELECT_MORE_NO_VIA, can_select_more);
    set_action_sensitive(ActionID::FILTER_AIRWIRES, can_select_more || n_pkgs);

    set_action_sensitive(ActionID::GO_TO_SCHEMATIC, sockets_connected);
    set_action_sensitive(ActionID::SHOW_IN_POOL_MANAGER, n_pkgs == 1 && sockets_connected);
    set_action_sensitive(ActionID::SHOW_IN_PROJECT_POOL_MANAGER, n_pkgs == 1 && sockets_connected);
    set_action_sensitive(ActionID::GO_TO_PROJECT_MANAGER, sockets_connected);
    set_action_sensitive(ActionID::OPEN_PROJECT, sel_count_type(sel, ObjectType::BOARD_PANEL) == 1);

    const auto n_planes = core_board.get_board()->planes.size();
    set_action_sensitive(ActionID::SELECT_PLANE, n_planes > 0);

    ImpBase::update_action_sensitivity();
}

void ImpBoard::apply_preferences()
{
    if (view_3d_window) {
        view_3d_window->apply_preferences(preferences);
    }
    canvas->set_highlight_on_top(preferences.board.highlight_on_top);
    canvas->show_text_in_tracks = preferences.board.show_text_in_tracks;
    canvas->show_text_in_vias = preferences.board.show_text_in_vias;
    canvas->show_via_span = preferences.board.show_via_span;
    ImpLayer::apply_preferences();
    canvas_update_from_pp();
}

static Gdk::RGBA rgba_from_color(const Color &c)
{
    Gdk::RGBA r;
    r.set_rgba(c.r, c.g, c.b);
    return r;
}

static Color color_from_rgba(const Gdk::RGBA &r)
{
    return {r.get_red(), r.get_green(), r.get_blue()};
}

int ImpBoard::get_schematic_pid()
{
    json j;
    j["op"] = "get-schematic-pid";
    return this->send_json(j).get<int>();
}

static json serialize_connector(const Track::Connection &conn)
{
    if (conn.is_pad() && conn.pad->net == nullptr) {
        const auto comp = conn.package->component;
        auto pm = comp->part->pad_map.at(conn.pad->uuid);
        UUIDPath<2> path(pm.gate->uuid, pm.pin->uuid);
        json j;
        j["component"] = (std::string)comp->uuid;
        j["path"] = (std::string)path;
        return j;
    }
    else if (conn.is_pad() && conn.pad->net) {
        json j;
        j["net"] = (std::string)conn.pad->net->uuid;
        return j;
    }
    else if (conn.is_junc() && conn.junc->net) {
        json j;
        j["net"] = (std::string)conn.junc->net->uuid;
        return j;
    }
    else {
        return nullptr;
    }
}

void ImpBoard::handle_select_more(const ActionConnection &conn)
{
    const bool no_via = conn.id.action == ActionID::SELECT_MORE_NO_VIA;
    std::map<const Junction *, std::set<const Track *>> junction_connections;
    const auto brd = core_board.get_board();
    for (const auto &it : brd->tracks) {
        for (const auto &it_ft : {it.second.from, it.second.to}) {
            if (it_ft.is_junc()) {
                junction_connections[it_ft.junc].insert(&it.second);
            }
        }
    }
    std::set<const Junction *> junctions;
    std::set<const Track *> tracks;

    auto sel = canvas->get_selection();

    for (const auto &it : sel) {
        if (it.type == ObjectType::TRACK) {
            tracks.insert(&brd->tracks.at(it.uuid));
        }
        else if (it.type == ObjectType::JUNCTION) {
            junctions.insert(&brd->junctions.at(it.uuid));
        }
        else if (it.type == ObjectType::VIA) {
            junctions.insert(brd->vias.at(it.uuid).junction);
        }
    }
    bool inserted = true;
    while (inserted) {
        inserted = false;
        for (const auto it : tracks) {
            for (const auto &it_ft : {it->from, it->to}) {
                if (it_ft.is_junc()) {
                    if (!no_via || (no_via && !it_ft.junc->has_via))
                        if (junctions.insert(it_ft.junc).second)
                            inserted = true;
                }
            }
        }
        for (const auto it : junctions) {
            if (junction_connections.count(it)) {
                for (const auto &it_track : junction_connections.at(it)) {
                    if (tracks.insert(it_track).second)
                        inserted = true;
                }
            }
        }
    }

    std::set<SelectableRef> new_sel;

    for (const auto it : junctions) {
        new_sel.emplace(it->uuid, ObjectType::JUNCTION);
    }
    for (const auto it : tracks) {
        new_sel.emplace(it->uuid, ObjectType::TRACK);
    }
    canvas->set_selection(new_sel);
    canvas->set_selection_mode(CanvasGL::SelectionMode::NORMAL);
}

void ImpBoard::handle_show_in_pool_manager(const ActionConnection &conn)
{
    const auto &board = *core_board.get_board();
    const auto pool_sel = conn.id.action == ActionID::SHOW_IN_PROJECT_POOL_MANAGER ? ShowInPoolManagerPool::CURRENT
                                                                                   : ShowInPoolManagerPool::LAST;
    for (const auto &it : canvas->get_selection()) {
        if (it.type == ObjectType::BOARD_PACKAGE) {
            const auto &pkg = board.packages.at(it.uuid);
            if (pkg.component->part) {
                show_in_pool_manager(ObjectType::PART, pkg.component->part->uuid, pool_sel);
                break;
            }
        }
    }
}

void ImpBoard::construct()
{
    ImpLayer::construct_layer_box(false);

    state_store = std::make_unique<WindowStateStore>(main_window, "imp-board");

    auto view_3d_button = Gtk::manage(new Gtk::Button("3D"));
    main_window->header->pack_start(*view_3d_button);
    view_3d_button->show();
    view_3d_button->signal_clicked().connect([this] {
        view_3d_window->update();
        view_3d_window->present();
    });

    hamburger_menu->append("Fabrication output", "win.fab_out");
    main_window->add_action("fab_out", [this] { trigger_action(ActionID::FAB_OUTPUT_WINDOW); });

    hamburger_menu->append("Export Pick & place…", "win.export_pnp");
    main_window->add_action("export_pnp", [this] { trigger_action(ActionID::PNP_EXPORT_WINDOW); });

    hamburger_menu->append("Export STEP…", "win.export_step");
    main_window->add_action("export_step", [this] { trigger_action(ActionID::STEP_EXPORT_WINDOW); });

    hamburger_menu->append("Export PDF…", "win.export_pdf");
    main_window->add_action("export_pdf", [this] { trigger_action(ActionID::PDF_EXPORT_WINDOW); });

    hamburger_menu->append("Import DXF…", "win.import_dxf");
    add_tool_action(ToolID::IMPORT_DXF, "import_dxf");

    hamburger_menu->append("Stackup", "win.edit_stackup");
    add_tool_action(ToolID::EDIT_STACKUP, "edit_stackup");

    hamburger_menu->append("Update all planes", "win.update_all_planes");
    add_tool_action(ToolID::UPDATE_ALL_PLANES, "update_all_planes");

    hamburger_menu->append("Part list", "win.part_list");
    main_window->add_action("part_list", [this] { trigger_action(ActionID::PARTS_WINDOW); });

    hamburger_menu->append("Length tuning", "win.tuning");
    main_window->add_action("tuning", [this] { trigger_action(ActionID::TUNING); });

    add_tool_action(ActionID::AIRWIRE_FILTER_WINDOW, "airwire_filter");
    view_options_menu_append_action("Nets…", "win.airwire_filter");

    view_options_menu_append_action("Bottom view", "win.bottom_view");
    add_view_angle_actions();

    if (sockets_connected) {
        hamburger_menu->append("Cross probing", "win.cross_probing");
        auto cp_action = main_window->add_action_bool("cross_probing", true);
        cross_probing_enabled = true;
        cp_action->signal_change_state().connect([this, cp_action](const Glib::VariantBase &v) {
            cross_probing_enabled = Glib::VariantBase::cast_dynamic<Glib::Variant<bool>>(v).get();
            g_simple_action_set_state(cp_action->gobj(), g_variant_new_boolean(cross_probing_enabled));
            if (!cross_probing_enabled && !core_board.tool_is_active()) {
                highlights.clear();
                update_highlights();
            }
        });

        connect_action(ActionID::GO_TO_SCHEMATIC, [this](const auto &conn) {
            json j;
            j["time"] = gtk_get_current_event_time();
            j["op"] = "present-schematic";
            auto sch_pid = this->get_schematic_pid();
            if (sch_pid != -1)
                allow_set_foreground_window(sch_pid);
            this->send_json(j);
        });

        connect_action(ActionID::SHOW_IN_POOL_MANAGER, sigc::mem_fun(*this, &ImpBoard::handle_show_in_pool_manager));
        connect_action(ActionID::SHOW_IN_PROJECT_POOL_MANAGER,
                       sigc::mem_fun(*this, &ImpBoard::handle_show_in_pool_manager));
        set_action_sensitive(ActionID::SHOW_IN_POOL_MANAGER, false);
        set_action_sensitive(ActionID::SHOW_IN_PROJECT_POOL_MANAGER, false);

        connect_action(ActionID::BACKANNOTATE_CONNECTION_LINES, [this](const auto &conn) {
            json j;
            j["time"] = gtk_get_current_event_time();
            j["op"] = "backannotate";
            allow_set_foreground_window(this->get_schematic_pid());
            json a;
            for (const auto &it : core_board.get_board()->connection_lines) {
                json x;
                x["from"] = serialize_connector(it.second.from);
                x["to"] = serialize_connector(it.second.to);
                if (!x["from"].is_null() && !x["to"].is_null())
                    a.push_back(x);
            }
            j["connections"] = a;
            allow_set_foreground_window(this->get_schematic_pid());
            this->send_json(j);
        });

        connect_go_to_project_manager_action();
    }

    connect_action(ActionID::RELOAD_NETLIST, [this](const ActionConnection &c) {
        reload_netlist_delay_conn.disconnect();
#if GTK_CHECK_VERSION(3, 22, 0)
        reload_netlist_popover->popdown();
#else
        reload_netlist_popover->hide();
#endif
        core_board.reload_netlist();
        core_board.set_needs_save();
        const auto sel = canvas->get_selection();
        canvas_update();
        airwire_filter_window->update_nets();
        parts_window->update();
        canvas->set_selection(sel);
    });

    {
        auto button = create_action_button(ActionID::RELOAD_NETLIST);
        button->show();
        main_window->header->pack_end(*button);

        reload_netlist_popover = Gtk::manage(new Gtk::Popover);
        reload_netlist_popover->set_modal(false);
        reload_netlist_popover->set_relative_to(*button);

        auto la = Gtk::manage(
                new Gtk::Label("Netlist has changed.\nReload the netlist to update the board to the latest netlist."));
        la->show();
        reload_netlist_popover->add(*la);
        la->set_line_wrap(true);
        la->set_max_width_chars(20);
        la->property_margin() = 10;
    }

    fab_output_window = FabOutputWindow::create(main_window, core_board, project_dir);
    core->signal_tool_changed().connect([this](ToolID t) { fab_output_window->set_can_generate(t == ToolID::NONE); });
    core->signal_rebuilt().connect([this] { fab_output_window->reload_layers(); });
    fab_output_window->signal_changed().connect([this] { core_board.set_needs_save(); });
    connect_action(ActionID::FAB_OUTPUT_WINDOW, [this](const auto &c) { fab_output_window->present(); });
    connect_action(ActionID::GEN_FAB_OUTPUT, [this](const auto &c) {
        fab_output_window->present();
        fab_output_window->generate();
    });

    pdf_export_window =
            PDFExportWindow::create(main_window, core_board, core_board.get_pdf_export_settings(), project_dir);
    pdf_export_window->signal_changed().connect([this] { core_board.set_needs_save(); });
    core->signal_rebuilt().connect([this] { pdf_export_window->reload_layers(); });
    connect_action(ActionID::PDF_EXPORT_WINDOW, [this](const auto &c) { pdf_export_window->present(); });
    connect_action(ActionID::EXPORT_PDF, [this](const auto &c) {
        pdf_export_window->present();
        pdf_export_window->generate();
    });

    view_3d_window = View3DWindow::create(*core_board.get_board(), *pool_caching, View3DWindow::Mode::BOARD);
    view_3d_window->set_solder_mask_color(rgba_from_color(core_board.get_colors().solder_mask));
    view_3d_window->set_silkscreen_color(rgba_from_color(core_board.get_colors().silkscreen));
    view_3d_window->set_substrate_color(rgba_from_color(core_board.get_colors().substrate));
    view_3d_window->signal_changed().connect([this] {
        core_board.get_colors().solder_mask = color_from_rgba(view_3d_window->get_solder_mask_color());
        core_board.get_colors().silkscreen = color_from_rgba(view_3d_window->get_silkscreen_color());
        core_board.get_colors().substrate = color_from_rgba(view_3d_window->get_substrate_color());
        core_board.set_needs_save();
    });
    view_3d_window->signal_present_imp().connect([this] { main_window->present(); });
    view_3d_window->signal_package_select().connect([this](const auto &uu) {
        if (!core->tool_is_active() && canvas->get_selection_mode() == CanvasGL::SelectionMode::NORMAL) {
            if (uu)
                canvas->set_selection({{uu, ObjectType::BOARD_PACKAGE}});
            else
                canvas->set_selection({});
        }
    });
    core_board.signal_rebuilt().connect([this] { view_3d_window->set_needs_update(); });

    step_export_window = StepExportWindow::create(main_window, core_board, project_dir);
    step_export_window->signal_changed().connect([this] { core_board.set_needs_save(); });
    connect_action(ActionID::STEP_EXPORT_WINDOW, [this](const auto &c) { step_export_window->present(); });
    connect_action(ActionID::EXPORT_STEP, [this](const auto &c) {
        step_export_window->present();
        step_export_window->generate();
    });

    tuning_window = new TuningWindow(*core_board.get_board());
    tuning_window->set_transient_for(*main_window);
    imp_interface->signal_request_length_tuning_ref().connect([this] { return tuning_window->get_ref_length(); });

    rules_window->signal_goto().connect(
            [this](Coordi location, UUID sheet, UUIDVec instance_path) { canvas->center_and_zoom(location); });

    connect_action(ActionID::VIEW_3D, [this](const auto &a) {
        view_3d_window->update();
        view_3d_window->present();
    });

    connect_action(ActionID::TUNING, [this](const auto &a) { tuning_window->present(); });
    connect_action(ActionID::TUNING_ADD_TRACKS, sigc::mem_fun(*this, &ImpBoard::handle_measure_tracks));
    connect_action(ActionID::TUNING_ADD_TRACKS_ALL, sigc::mem_fun(*this, &ImpBoard::handle_measure_tracks));

    parts_window = new PartsWindow(*core_board.get_board());
    parts_window->set_transient_for(*main_window);
    connect_action(ActionID::PARTS_WINDOW, [this](const auto &a) { parts_window->present(); });
    parts_window->update();
    parts_window->signal_selected().connect([this](const auto &comps) {
        highlights.clear();
        std::set<UUID> pkgs;
        for (const auto &[uu, pkg] : core_board.get_board()->packages) {
            if (comps.count(pkg.component->uuid)) {
                highlights.emplace(ObjectType::BOARD_PACKAGE, uu);
                pkgs.insert(uu);
            }
        }
        update_highlights();
        view_3d_window->set_highlights(pkgs);
    });
    if (m_meta.count("parts"))
        parts_window->load_from_json(m_meta.at("parts"));

    connect_action(ActionID::HIGHLIGHT_NET, [this](const auto &a) {
        highlights.clear();
        for (const auto &it : canvas->get_selection()) {
            if (auto uu = net_from_selectable(it)) {
                highlights.emplace(ObjectType::NET, uu);
            }
        }
        this->update_highlights();
    });

    connect_action(ActionID::HIGHLIGHT_NET_CLASS, [this](const auto &a) {
        highlights.clear();
        for (const auto &it : canvas->get_selection()) {
            if (auto uu = net_from_selectable(it)) {
                const auto &net_sel = core_board.get_top_block()->nets.at(uu);
                for (const auto &[net_uu, net] : core_board.get_top_block()->nets) {
                    if (net.net_class == net_sel.net_class)
                        highlights.emplace(ObjectType::NET, net_uu);
                }
            }
        }
        this->update_highlights();
    });

    connect_action(ActionID::SELECT_MORE, sigc::mem_fun(*this, &ImpBoard::handle_select_more));
    connect_action(ActionID::SELECT_MORE_NO_VIA, sigc::mem_fun(*this, &ImpBoard::handle_select_more));

    auto *display_control_notebook = Gtk::manage(new Gtk::Notebook);
    display_control_notebook->append_page(*layer_box, "Layers");
    layer_box->show();
    layer_box->get_style_context()->add_class("background");

    board_display_options_box = Gtk::manage(new BoardDisplayOptionsBox(core_board.get_layer_provider()));
    {
        auto fbox = Gtk::manage(new Gtk::Box());
        fbox->pack_start(*board_display_options_box, true, true, 0);
        fbox->get_style_context()->add_class("background");
        fbox->show();
        display_control_notebook->append_page(*fbox, "Options");
        board_display_options_box->show();
    }

    board_display_options_box->signal_set_layer_display().connect([this](int index, const LayerDisplay &lda) {
        LayerDisplay ld = canvas->get_layer_display(index);
        ld.types_visible = lda.types_visible;
        canvas->set_layer_display(index, ld);
    });
    canvas->set_layer_display(10000, LayerDisplay(true, LayerDisplay::Mode::OUTLINE));
    core->signal_rebuilt().connect([this] { board_display_options_box->update(); });
    if (m_meta.count("board_display_options"))
        board_display_options_box->load_from_json(m_meta.at("board_display_options"));

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

    text_owner_annotation = canvas->create_annotation();
    text_owner_annotation->set_visible(true);
    text_owner_annotation->set_display(LayerDisplay(true, LayerDisplay::Mode::OUTLINE));

    airwire_annotation = canvas->create_annotation();
    airwire_annotation->set_visible(true);
    airwire_annotation->set_display(LayerDisplay(true, LayerDisplay::Mode::OUTLINE));
    airwire_annotation->use_highlight = true;

    core_board.signal_rebuilt().connect(sigc::mem_fun(*this, &ImpBoard::update_text_owners));
    canvas->signal_hover_selection_changed().connect(sigc::mem_fun(*this, &ImpBoard::update_text_owner_annotation));
    canvas->signal_selection_changed().connect(sigc::mem_fun(*this, &ImpBoard::update_text_owner_annotation));
    update_text_owners();

    core_board.signal_rebuilt().connect([this] { selection_filter_dialog->update_layers(); });

    main_window->left_panel->pack_start(*display_control_notebook, false, false);

    unplaced_box = Gtk::manage(new UnplacedBox("Package"));
    unplaced_box->show();
    main_window->left_panel->pack_end(*unplaced_box, true, true, 0);
    unplaced_box->signal_place().connect([this](const auto &items) {
        std::set<SelectableRef> components;
        for (const auto &it : items) {
            components.emplace(it.at(0), ObjectType::COMPONENT);
        }
        this->tool_begin(ToolID::MAP_PACKAGE, true, components);
    });
    unplaced_box->signal_selected().connect([this](const auto &items) {
        json j;
        j["op"] = "board-select";
        j["selection"] = nullptr;
        for (const auto &it : items) {
            json k;
            k["type"] = static_cast<int>(ObjectType::COMPONENT);
            k["uuid"] = (std::string)it.at(0);
            j["selection"].push_back(k);
        }
        send_json(j);
    });
    core_board.signal_rebuilt().connect(sigc::mem_fun(*this, &ImpBoard::update_unplaced));
    update_unplaced();
    core_board.signal_tool_changed().connect(
            [this](ToolID tool_id) { unplaced_box->set_sensitive(tool_id == ToolID::NONE); });


    pnp_export_window = PnPExportWindow::create(main_window, *core_board.get_board(),
                                                core_board.get_pnp_export_settings(), project_dir);

    connect_action(ActionID::PNP_EXPORT_WINDOW, [this](const auto &c) {
        pnp_export_window->update();
        pnp_export_window->present();
    });
    connect_action(ActionID::EXPORT_PNP, [this](const auto &c) {
        pnp_export_window->update();
        pnp_export_window->present();
        pnp_export_window->generate();
    });

    pnp_export_window->signal_changed().connect([this] { core_board.set_needs_save(); });
    core->signal_tool_changed().connect([this](ToolID t) { pnp_export_window->set_can_export(t == ToolID::NONE); });
    core->signal_rebuilt().connect([this] {
        if (pnp_export_window->get_visible())
            pnp_export_window->update();
    });

    airwire_filter_window = AirwireFilterWindow::create(main_window, *core_board.get_board());
    airwire_filter_window->update_nets();
    airwire_filter_window->signal_changed().connect([this] {
        update_net_colors();
        apply_net_colors();
        update_airwire_annotation();
        update_view_hints();
    });
    airwire_filter_window->signal_selection_changed().connect([this](auto nets) {
        highlights.clear();
        for (const auto &net : nets) {
            highlights.emplace(ObjectType::NET, net);
        }
        update_highlights();
    });
    connect_action(ActionID::AIRWIRE_FILTER_WINDOW, [this](const auto &a) { airwire_filter_window->present(); });
    core_board.signal_rebuilt().connect(sigc::mem_fun(*this, &ImpBoard::update_airwires));
    connect_action(ActionID::RESET_AIRWIRE_FILTER, [this](const auto &a) { airwire_filter_window->set_all(true); });
    connect_action(ActionID::FILTER_AIRWIRES, [this](const auto &a) {
        std::set<UUID> nets;
        const auto board = core_board.get_board();
        for (const auto &it : canvas->get_selection()) {
            switch (it.type) {
            case ObjectType::TRACK: {
                auto &track = board->tracks.at(it.uuid);
                if (track.net) {
                    nets.emplace(track.net->uuid);
                }
            } break;
            case ObjectType::VIA: {
                auto &via = board->vias.at(it.uuid);
                if (via.junction->net) {
                    nets.emplace(via.junction->net->uuid);
                }
            } break;
            case ObjectType::JUNCTION: {
                auto &ju = board->junctions.at(it.uuid);
                if (ju.net) {
                    nets.emplace(ju.net->uuid);
                }
            } break;
            case ObjectType::BOARD_PACKAGE: {
                auto &pkg = board->packages.at(it.uuid);
                for (const auto &it_pad : pkg.package.pads) {
                    if (it_pad.second.net) {
                        nets.emplace(it_pad.second.net->uuid);
                    }
                }
            } break;
            default:;
            }
        }

        airwire_filter_window->set_only(nets);
    });

    layer_box->property_layer_mode().signal_changed().connect([this] { update_airwire_annotation(); });
    canvas->property_work_layer().signal_changed().connect([this] { update_airwire_annotation(); });
    layer_box->signal_set_layer_display().connect([this](auto, auto) { update_airwire_annotation(); });

    connect_action(ActionID::OPEN_DATASHEET, [this](const auto &a) {
        const auto x = sel_find_exactly_one(canvas->get_selection(), ObjectType::BOARD_PACKAGE);
        if (x && core_board.get_board()->packages.count(x->uuid)) {
            auto part = core_board.get_board()->packages.at(x->uuid).component->part;
            if (part && part->get_datasheet().size())
                gtk_show_uri_on_window(GTK_WINDOW(main_window->gobj()), part->get_datasheet().c_str(), GDK_CURRENT_TIME,
                                       NULL);
        }
    });

    connect_action(ActionID::OPEN_PROJECT, [this](const auto &a) {
        const auto x = sel_find_exactly_one(canvas->get_selection(), ObjectType::BOARD_PANEL);
        const auto &brd = *core_board.get_board();
        if (x && brd.board_panels.count(x->uuid)) {
            const auto &p = brd.board_panels.at(x->uuid);
            json j;
            j["op"] = "open-project";
            j["filename"] = p.included_board->get_absolute_project_filename(brd.board_directory);
            this->send_json(j);
        }
    });

    connect_action(ActionID::SELECT_PLANE, [this](const auto &a) {
        const auto pos = canvas->get_cursor_pos();
        const auto &brd = *core_board.get_board();
        std::vector<std::pair<const Polygon *, int64_t>> candidates;
        for (auto &[uu, it] : brd.planes) {
            if (!canvas->layer_is_visible(it.polygon->layer))
                continue;

            for (auto &frag : it.fragments) {
                if (frag.contains(pos))
                    candidates.emplace_back(it.polygon, frag.get_area());
            }
        }

        if (!candidates.size())
            return;

        std::sort(candidates.begin(), candidates.end(),
                  [](const auto &a, const auto &b) { return a.second < b.second; });

        const auto &poly = candidates.front();

        canvas->set_selection({{poly.first->uuid, ObjectType::POLYGON_VERTEX, 0}});
        canvas->set_selection_mode(CanvasGL::SelectionMode::NORMAL);
    });

    if (m_meta.count("nets"))
        airwire_filter_window->load_from_json(m_meta.at("nets"));

    {
        auto &b = add_action_button(ToolID::ROUTE_TRACK_INTERACTIVE);
        b.add_action(ToolID::ROUTE_DIFFPAIR_INTERACTIVE);
    }
    add_action_button_polygon();
    {
        auto &b = add_action_button(ToolID::PLACE_BOARD_HOLE);
        b.add_action(ToolID::PLACE_VIA);
    }
    add_action_button_line();

    add_action_button(ToolID::PLACE_TEXT);
    add_action_button(ToolID::DRAW_DIMENSION);

    update_monitor();

    display_control_notebook->show();

    set_view_angle(0);
}

void ImpBoard::set_window_title(const std::string &s)
{
    view_3d_window->set_3d_title(s);
    ImpBase::set_window_title(s);
}

UUID ImpBoard::net_from_selectable(const SelectableRef &sr)
{
    const auto &board = *core_board.get_board();
    switch (sr.type) {
    case ObjectType::TRACK: {
        auto &track = board.tracks.at(sr.uuid);
        if (track.net) {
            return track.net->uuid;
        }
    } break;
    case ObjectType::VIA: {
        auto &via = board.vias.at(sr.uuid);
        if (via.junction->net) {
            return via.junction->net->uuid;
        }
    } break;
    case ObjectType::JUNCTION: {
        auto &ju = board.junctions.at(sr.uuid);
        if (ju.net) {
            return ju.net->uuid;
        }
    } break;
    case ObjectType::POLYGON_ARC_CENTER:
    case ObjectType::POLYGON_EDGE:
    case ObjectType::POLYGON_VERTEX: {
        auto &poly = board.polygons.at(sr.uuid);
        if (auto plane = dynamic_cast<const Plane *>(poly.usage.ptr)) {
            return plane->net->uuid;
        }
    } break;
    default:;
    }
    return UUID();
}

void ImpBoard::update_airwires()
{
    airwire_filter_window->update_from_board();
}

void ImpBoard::update_text_owners()
{
    auto brd = core_board.get_board();
    for (const auto &it_pkg : brd->packages) {
        for (const auto &itt : it_pkg.second.texts) {
            text_owners[itt->uuid] = it_pkg.first;
        }
    }
    update_text_owner_annotation();
}

void ImpBoard::update_text_owner_annotation()
{
    text_owner_annotation->clear();
    auto sel = canvas->get_selection();
    const auto brd = core_board.get_board();
    for (const auto &it : sel) {
        if (it.type == ObjectType::TEXT) {
            if (brd->texts.count(it.uuid)) {
                const auto &text = brd->texts.at(it.uuid);
                if (text_owners.count(text.uuid))
                    text_owner_annotation->draw_line(text.placement.shift,
                                                     brd->packages.at(text_owners.at(text.uuid)).placement.shift,
                                                     ColorP::FROM_LAYER, 0);
            }
        }
        else if (it.type == ObjectType::BOARD_PACKAGE) {
            if (brd->packages.count(it.uuid)) {
                const auto &pkg = brd->packages.at(it.uuid);
                for (const auto &text : pkg.texts) {
                    if (canvas->layer_is_visible(text->layer))
                        text_owner_annotation->draw_line(text->placement.shift, pkg.placement.shift, ColorP::FROM_LAYER,
                                                         0);
                }
            }
        }
    }
}

std::string ImpBoard::get_hud_text(std::set<SelectableRef> &sel)
{
    std::string s;
    if (sel_has_type(sel, ObjectType::TRACK)) {
        auto n = sel_count_type(sel, ObjectType::TRACK);
        s += "\n\n<b>" + std::to_string(n) + " " + object_descriptions.at(ObjectType::TRACK).get_name_for_n(n)
             + "</b>\n";
        std::set<int> layers;
        std::set<const Net *> nets;
        std::vector<const Track *> tracks;
        int64_t length = 0;
        const Track *the_track = nullptr;
        for (const auto &it : sel) {
            if (it.type == ObjectType::TRACK) {
                const auto &tr = core_board.get_board()->tracks.at(it.uuid);
                the_track = &tr;
                tracks.emplace_back(the_track);
                layers.insert(tr.layer);
                length += tr.get_length();
                if (tr.net)
                    nets.insert(tr.net);
            }
        }
        s += "Layers: " + get_hud_text_for_layers(layers);
        s += "\nTotal length: " + dim_to_string(length, false);

        if (n == 2) {
            // Show spacing between 2 parallel tracks
            const auto t1 = *tracks.at(0);
            const auto t2 = *tracks.at(1);
            if (t1.is_parallel_to(t2)) {
                const Coordd u = t1.to.get_position() - t1.from.get_position();
                const Coordd v = u.normalize();
                const Coordd w = t2.from.get_position() - t1.from.get_position();
                const int64_t offset = v.cross(w);
                s += "\nSpacing: " + dim_to_string(offset, false);
            }
        }

        if (n == 1) {
            s += "\n" + get_hud_text_for_net(the_track->net);
        }
        else {
            s += "\n" + std::to_string(nets.size()) + " "
                 + object_descriptions.at(ObjectType::NET).get_name_for_n(nets.size());
        }
        sel_erase_type(sel, ObjectType::TRACK);
    }
    trim(s);
    if (auto it_sel = sel_find_exactly_one(sel, ObjectType::BOARD_PACKAGE)) {
        const auto &pkg = core_board.get_board()->packages.at(it_sel->uuid);
        s += "\n\n<b>Package " + Glib::Markup::escape_text(pkg.component->refdes) + "</b>";
        if (pkg.fixed) {
            s += " (not movable)";
        }
        s += "\n";
        s += get_hud_text_for_component(pkg.component);
        sel_erase_type(sel, ObjectType::BOARD_PACKAGE);
    }
    else if (sel_count_type(sel, ObjectType::BOARD_PACKAGE) > 1) {
        auto n = sel_count_type(sel, ObjectType::BOARD_PACKAGE);
        s += "\n\n<b>" + std::to_string(n) + " " + object_descriptions.at(ObjectType::BOARD_PACKAGE).get_name_for_n(n)
             + "</b>\n";
        size_t n_pads = 0;
        for (const auto &it : sel) {
            if (it.type == ObjectType::BOARD_PACKAGE) {
                const auto &pkg = core_board.get_board()->packages.at(it.uuid);
                n_pads += pkg.package.pads.size();
            }
        }
        s += "Total pads: " + std::to_string(n_pads);

        // When n == 2 selection is removed when showing delta below
        if (n != 2) {
            sel_erase_type(sel, ObjectType::BOARD_PACKAGE);
        }
    }
    trim(s);
    if (sel_count_type(sel, ObjectType::POLYGON_VERTEX) == 1 || sel_count_type(sel, ObjectType::POLYGON_EDGE) == 1) {
        const auto &brd = *core_board.get_board();
        const Polygon *poly = nullptr;
        if (sel_has_type(sel, ObjectType::POLYGON_VERTEX))
            poly = &brd.polygons.at(sel_find_one(sel, ObjectType::POLYGON_VERTEX).uuid);
        if (sel_has_type(sel, ObjectType::POLYGON_EDGE))
            poly = &brd.polygons.at(sel_find_one(sel, ObjectType::POLYGON_EDGE).uuid);
        if (poly) {
            if (auto plane = dynamic_cast<const Plane *>(poly->usage.ptr)) {
                s += "\n\n<b>Plane " + Glib::Markup::escape_text(plane->net->name) + "</b>\n";
                s += "Fill order: " + std::to_string(plane->priority) + "\n";
                s += "Layer: ";
                s += core_board.get_layer_provider().get_layers().at(poly->layer).name + "\n";
            }
        }
    }
    trim(s);

    // Display the delta if two items of these types are selected
    for (const ObjectType type : {ObjectType::BOARD_PACKAGE, ObjectType::BOARD_HOLE, ObjectType::VIA}) {
        if (sel_count_type(sel, type) == 2) {
            // Already added for packages
            if (type != ObjectType::BOARD_PACKAGE) {
                s += "\n\n<b>2 " + object_descriptions.at(type).name_pl + "</b>";
            }
            std::vector<Coordi> positions;
            const auto &brd = *core_board.get_board();
            for (const auto &it : sel) {
                if (it.type == type) {
                    if (type == ObjectType::BOARD_HOLE) {
                        const auto &hole = &brd.holes.at(it.uuid);
                        positions.push_back(hole->placement.shift);
                    }
                    else if (type == ObjectType::VIA) {
                        const auto &via = &brd.vias.at(it.uuid);
                        positions.push_back(via->junction->position);
                    }
                    else if (type == ObjectType::BOARD_PACKAGE) {
                        const auto &package = &brd.packages.at(it.uuid);
                        positions.push_back(package->placement.shift);
                    }
                    else {
                        assert(false); // unreachable
                    }
                }
            }
            assert(positions.size() == 2);
            const auto delta = positions.at(1) - positions.at(0);
            s += "\n" + coord_to_string(delta, true);
            sel_erase_type(sel, type);
        }
    }
    trim(s);

    return s;
}

void ImpBoard::handle_measure_tracks(const ActionConnection &a)
{
    auto sel = canvas->get_selection();
    std::set<UUID> tracks;
    for (const auto &it : sel) {
        if (it.type == ObjectType::TRACK) {
            tracks.insert(it.uuid);
        }
    }
    tuning_window->add_tracks(tracks, a.id.action == ActionID::TUNING_ADD_TRACKS_ALL);
    tuning_window->present();
}

ToolID ImpBoard::get_tool_for_drag_move(bool ctrl, const std::set<SelectableRef> &sel) const
{
    if (preferences.board.move_using_router && sel.size() == 1 && sel_count_type(sel, ObjectType::TRACK) == 1) {
        auto &track = core_board.get_board()->tracks.at(sel_find_one(sel, ObjectType::TRACK).uuid);
        if (track.is_arc())
            return ToolID::DRAG_KEEP_SLOPE;
        else
            return ToolID::DRAG_TRACK_INTERACTIVE;
    }
    else {
        return ImpBase::get_tool_for_drag_move(ctrl, sel);
    }
}

void ImpBoard::handle_maybe_drag(bool ctrl)
{
    if (!preferences.board.drag_start_track) {
        ImpBase::handle_maybe_drag(ctrl);
        return;
    }
    auto target = canvas->get_current_target();
    const auto brd = core_board.get_board();
    if (target.type == ObjectType::PAD) {
        auto &pkg = brd->packages.at(target.path.at(0));
        auto &pad = pkg.package.pads.at(target.path.at(1));
        if (pad.padstack.type == Padstack::Type::MECHANICAL || pad.is_nc) {
            ImpBase::handle_maybe_drag(ctrl);
            return;
        }
    }
    else if (target.type == ObjectType::JUNCTION) {
        const auto &ju = brd->junctions.at(target.path.at(0));
        if (!ju.net) {
            ImpBase::handle_maybe_drag(ctrl);
            return;
        }
    }
    else {
        ImpBase::handle_maybe_drag(ctrl);
        return;
    }
    if (!canvas->selection_filter.can_select(SelectableRef(UUID(), ObjectType::TRACK, 0, target.layer))) {
        ImpBase::handle_maybe_drag(ctrl);
        return;
    }
    canvas->inhibit_drag_selection();
    target_drag_begin = target;
    cursor_pos_drag_begin = canvas->get_cursor_pos_win();
}

void ImpBoard::handle_drag()
{
    auto pos = canvas->get_cursor_pos_win();
    auto delta = pos - cursor_pos_drag_begin;
    auto brd = core_board.get_board();
    bool have_net = false;
    if (target_drag_begin.type == ObjectType::PAD) {
        auto &pkg = brd->packages.at(target_drag_begin.path.at(0));
        auto &pad = pkg.package.pads.at(target_drag_begin.path.at(1));
        have_net = pad.net;
    }
    else if (target_drag_begin.type == ObjectType::JUNCTION) {
        have_net = brd->junctions.at(target_drag_begin.path.at(0)).net;
    }

    if (delta.mag_sq() > (50 * 50)) {
        if (have_net) {
            {
                highlights.clear();
                update_highlights();
                ToolArgs args;
                args.coords = target_drag_begin.p;
                ToolResponse r = core->tool_begin(ToolID::ROUTE_TRACK_INTERACTIVE, args, imp_interface.get());
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
        }
        else {
            {
                highlights.clear();
                update_highlights();
                ToolArgs args;
                args.coords = target_drag_begin.p;
                ToolResponse r = core->tool_begin(ToolID::DRAW_CONNECTION_LINE, args, imp_interface.get());
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
        }
        target_drag_begin = Target();
    }
}

ActionToolID ImpBoard::get_doubleclick_action(ObjectType type, const UUID &uu)
{
    auto a = ImpBase::get_doubleclick_action(type, uu);
    if (a.action != ActionID::NONE)
        return a;
    switch (type) {
    case ObjectType::BOARD_HOLE:
        return ToolID::EDIT_BOARD_HOLE;
        break;
    case ObjectType::VIA:
        return ToolID::EDIT_VIA;
        break;
    case ObjectType::TRACK:
        return ActionID::SELECT_MORE;
        break;
    case ObjectType::POLYGON:
    case ObjectType::POLYGON_EDGE:
    case ObjectType::POLYGON_VERTEX: {
        auto poly = core_board.get_polygon(uu);
        if (poly->usage->is_type<Plane>())
            return ToolID::EDIT_PLANE;
        else if (poly->usage->is_type<Keepout>())
            return ToolID::EDIT_KEEPOUT;
        else
            return {ActionID::NONE, ToolID::NONE};
    } break;
    default:
        return {ActionID::NONE, ToolID::NONE};
    }
}

std::map<ObjectType, ImpBase::SelectionFilterInfo> ImpBoard::get_selection_filter_info() const
{
    const auto layers_sorted = core_board.get_board()->get_layers_sorted(LayerProvider::LayerSortOrder::TOP_TO_BOTTOM);

    std::vector<int> layers_track;
    std::vector<int> layers_line;
    std::vector<int> layers_polygon;

    for (const auto &layer : layers_sorted) {
        if (layer.copper) {
            layers_track.push_back(layer.index);
            layers_polygon.push_back(layer.index);
        }
        if (BoardLayers::is_user(layer.index)) {
            layers_line.push_back(layer.index);
            layers_polygon.push_back(layer.index);
        }
        switch (layer.index) {
        case BoardLayers::OUTLINE_NOTES:
            layers_line.push_back(layer.index);
            break;

        case BoardLayers::L_OUTLINE:
            layers_line.push_back(layer.index);
            break;

        case BoardLayers::TOP_ASSEMBLY:
        case BoardLayers::BOTTOM_ASSEMBLY:
            layers_line.push_back(layer.index);
            break;

        case BoardLayers::TOP_SILKSCREEN:
        case BoardLayers::BOTTOM_SILKSCREEN:
            layers_line.push_back(layer.index);
            layers_polygon.push_back(layer.index);
            break;

        case BoardLayers::TOP_MASK:
        case BoardLayers::BOTTOM_MASK:
            layers_line.push_back(layer.index);
            layers_polygon.push_back(layer.index);
            break;

        case BoardLayers::TOP_COPPER:
        case BoardLayers::BOTTOM_COPPER:
            layers_line.push_back(layer.index);
            break;
        }
    }

    std::vector<int> layers_package = {BoardLayers::TOP_COPPER, BoardLayers::BOTTOM_COPPER};

    using Flag = ImpBase::SelectionFilterInfo::Flag;
    std::map<ObjectType, ImpBase::SelectionFilterInfo> r = {
            {ObjectType::BOARD_PACKAGE, {layers_package, Flag::DEFAULT}},
            {ObjectType::TRACK, {layers_track, Flag::DEFAULT}},
            {ObjectType::BOARD_NET_TIE, {layers_track, Flag::DEFAULT}},
            {ObjectType::VIA, {layers_track, Flag::DEFAULT}},
            {ObjectType::BOARD_DECAL, {}},
            {ObjectType::POLYGON, {layers_polygon, Flag::HAS_OTHERS}},
            {ObjectType::TEXT, {layers_line, Flag::HAS_OTHERS}},
            {ObjectType::LINE, {layers_line, Flag::HAS_OTHERS}},
            {ObjectType::JUNCTION, {layers_track, Flag::HAS_OTHERS}},
            {ObjectType::ARC, {layers_line, Flag::HAS_OTHERS}},
            {ObjectType::DIMENSION, {}},
            {ObjectType::BOARD_HOLE, {{}, Flag::WORK_LAYER_ONLY_ENABLED}},
            {ObjectType::CONNECTION_LINE, {}},
            {ObjectType::BOARD_PANEL, {}},
            {ObjectType::PICTURE, {}},
    };
    return r;
}

void ImpBoard::update_unplaced()
{
    std::map<UUIDPath<2>, std::string> components;
    const auto brd = core_board.get_board();
    for (const auto &it : brd->block->components) {
        if (it.second.part)
            components.emplace(std::piecewise_construct, std::forward_as_tuple(it.second.uuid),
                               std::forward_as_tuple(it.second.refdes));
    }

    for (auto &it : brd->packages) {
        components.erase(it.second.component->uuid);
    }
    unplaced_box->update(components);
}

void ImpBoard::get_save_meta(json &j)
{
    ImpLayer::get_save_meta(j);
    j["board_display_options"] = board_display_options_box->serialize();
    j["nets"] = airwire_filter_window->serialize();
    j["parts"] = parts_window->serialize();
}

std::vector<std::string> ImpBoard::get_view_hints()
{
    auto r = ImpLayer::get_view_hints();

    if (airwire_filter_window->get_filtered())
        r.emplace_back("airwires filtered");

    return r;
}

void ImpBoard::update_net_colors()
{
    auto &net_colors = airwire_filter_window->get_net_colors();
    net_color_map.clear();
    if (net_colors.size()) {
        std::vector<ColorI> t;
        t.emplace_back(ColorI{0, 0, 0});

        std::map<ColorI, int> colors_idxs;
        auto get_or_create_color = [&t, &colors_idxs](ColorI c) -> int {
            if (colors_idxs.count(c)) {
                return colors_idxs.at(c);
            }
            else {
                t.emplace_back(c);
                const auto i = t.size() - 1;
                colors_idxs.emplace(c, i);
                return i;
            }
        };

        for (const auto &[net, color] : net_colors) {
            net_color_map[net] = get_or_create_color(color);
        }
        canvas->set_colors2(t);
    }
}


void ImpBoard::update_monitor()
{
    ItemSet mon_items = core_board.get_top_block()->get_pool_items_used();
    {
        ItemSet items = core_board.get_board()->get_pool_items_used();
        mon_items.insert(items.begin(), items.end());
    }
    set_monitor_items(mon_items);
}

ImpBoard::~ImpBoard()
{
    reload_netlist_delay_conn.disconnect();
    delete view_3d_window;
}

} // namespace horizon
