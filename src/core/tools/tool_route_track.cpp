#include "tool_route_track.hpp"
#include "board/board_rules.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include "imp/imp_interface.hpp"
#include "util/util.hpp"
#include <iostream>
#include <sstream>
#include <gdk/gdkkeysyms.h>

namespace horizon {

ToolRouteTrack::ToolRouteTrack(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

bool ToolRouteTrack::can_begin()
{
    return doc.b;
}

ToolResponse ToolRouteTrack::begin(const ToolArgs &args)
{
    std::cout << "tool route track\n";
    selection.clear();
    update_tip();
    rules = dynamic_cast<BoardRules *>(doc.r->get_rules());
    canvas_patch.update(*doc.b->get_board());

    return ToolResponse();
}

void ToolRouteTrack::begin_track(const ToolArgs &args)
{
    Coordi c;
    if (args.target.is_valid())
        c = args.target.p;
    else
        c = args.coords;

    routing_layer = args.work_layer;
    routing_width = rules->get_default_track_width(net, routing_layer);
    obstacles.clear();
    update_obstacles();
    track_path.clear();
    track_path.emplace_back(c.x, c.y);
    track_path.emplace_back(c.x, c.y);
    track_path.emplace_back(c.x, c.y);
    track_path_known_good = track_path;
}

void ToolRouteTrack::update_obstacles()
{
    obstacles.clear();
    canvas_patch.update(*doc.b->get_board());

    ClipperLib::Clipper clipper;

    for (const auto &it : canvas_patch.get_patches()) {
        if (it.first.layer == routing_layer && it.first.net != net->uuid
            && !(it.first.type == PatchType::TRACK && it.first.net == UUID())) { // patch is an obstacle
            ClipperLib::ClipperOffset ofs;
            ofs.ArcTolerance = 10e3;
            ofs.AddPaths(it.second, ClipperLib::jtRound, ClipperLib::etClosedPolygon);
            auto clearance = rules->get_clearance_copper(doc.b->get_block()->get_net(it.first.net), net, routing_layer);
            auto expand = clearance->get_clearance(it.first.type, PatchType::TRACK) + clearance->routing_offset;

            ClipperLib::Paths t_ofs;
            ofs.Execute(t_ofs, expand + routing_width / 2);
            clipper.AddPaths(t_ofs, ClipperLib::ptSubject, true);
        }
    }
    clipper.Execute(ClipperLib::ctUnion, obstacles, ClipperLib::pftNonZero);
    doc.b->get_board()->obstacles = obstacles;
}

void ToolRouteTrack::update_track(const Coordi &c)
{
    assert(track_path.size() >= 3);
    auto l = track_path.end() - 1;
    l->X = c.x;
    l->Y = c.y;

    auto k = l - 1;
    auto b = l - 2;
    auto dx = std::abs(l->X - b->X);
    auto dy = std::abs(l->Y - b->Y);
    if (dy > dx) {
        auto si = (l->Y < b->Y) ? 1 : -1;
        if (bend_mode) {
            k->X = b->X;
            k->Y = l->Y + si * dx;
        }
        else {
            k->X = l->X;
            k->Y = b->Y - si * dx;
        }
    }
    else {
        auto si = (l->X < b->X) ? 1 : -1;
        if (bend_mode) {
            k->Y = b->Y;
            k->X = l->X + si * dy;
        }
        else {
            k->Y = l->Y;
            k->X = b->X - si * dy;
        }
    }

    doc.b->get_board()->track_path = track_path;
}

bool ToolRouteTrack::check_track_path(const ClipperLib::Path &p)
{
    ClipperLib::Clipper clipper;
    clipper.AddPaths(obstacles, ClipperLib::ptClip, true);
    clipper.AddPath(p, ClipperLib::ptSubject, false);
    ClipperLib::PolyTree solution;
    clipper.Execute(ClipperLib::ctIntersection, solution, ClipperLib::pftNonZero, ClipperLib::pftNonZero);
    return solution.ChildCount() == 0;
}

static Coordi coordi_fron_intpt(const ClipperLib::IntPoint &p)
{
    Coordi r(p.X, p.Y);
    return r;
}

void ToolRouteTrack::update_temp_track()
{
    auto brd = doc.b->get_board();
    for (auto &it : temp_tracks) {
        brd->tracks.erase(it->uuid);
    }
    temp_tracks.clear();
    for (auto &it : temp_junctions) {
        brd->junctions.erase(it->uuid);
    }
    temp_junctions.clear();
    if (track_path.size() >= 3) {

        ClipperLib::Path path_simp;
        path_simp.push_back(track_path.at(0));
        path_simp.push_back(track_path.at(1));
        path_simp.push_back(track_path.at(2));

        for (auto it = track_path.cbegin() + 3; it < track_path.cend(); it++) {
            if (path_simp.back() == *it)
                continue;
            path_simp.push_back(*it);
            auto p0 = coordi_fron_intpt(*(path_simp.end() - 1));
            auto p1 = coordi_fron_intpt(*(path_simp.end() - 2));
            auto p2 = coordi_fron_intpt(*(path_simp.end() - 3));

            auto va = p2 - p1;
            auto vb = p1 - p0;

            if ((va.dot(vb)) * (va.dot(vb)) == va.mag_sq() * vb.mag_sq()) {
                path_simp.erase(path_simp.end() - 2);
            }
        }

        /*auto tj = doc.r->insert_junction(UUID::random());
        tj->temp = true;
        tj->net = net;
        tj->net_segment = net_segment;
        tj->position = coordi_fron_intpt(path_simp.at(1));
        temp_junctions.push_back(tj);

        auto uu = UUID::random();
        auto tt = &brd->tracks.emplace(uu, uu).first->second;
        tt->from = conn_start;
        tt->to.connect(tj);
        tt->net = net;
        tt->net_segment = net_segment;
        tt->layer = routing_layer;
        temp_tracks.push_back(tt);*/

        Junction *tj = nullptr;
        for (auto it = path_simp.cbegin() + 1; it < path_simp.cend(); it++) {
            if (*it != *(it - 1)) {
                auto tuu = UUID::random();
                auto tr = &brd->tracks.emplace(tuu, tuu).first->second;
                tr->net = net;
                tr->layer = routing_layer;
                tr->width = routing_width;
                tr->width_from_rules = routing_width_use_default;
                temp_tracks.push_back(tr);

                auto ju = doc.r->insert_junction(UUID::random());
                imp->set_snap_filter({{ObjectType::JUNCTION, ju->uuid}});
                ju->position = coordi_fron_intpt(*it);
                ju->net = net;
                temp_junctions.push_back(ju);

                if (tj == nullptr) {
                    tr->from = conn_start;
                }
                else {
                    tr->from.connect(tj);
                }

                tr->to.connect(ju);
                tj = ju;
            }
        }
    }
    if (via) {
        via->junction = temp_junctions.back();
    }

    doc.b->get_board()->update_airwires();
}

bool ToolRouteTrack::try_move_track(const ToolArgs &args)
{
    bool drc_okay = false;
    update_track(args.coords);
    if (!check_track_path(track_path)) { // drc error
        std::cout << "drc error" << std::endl;
        bend_mode ^= true;
        update_track(args.coords);           // flip
        if (!check_track_path(track_path)) { // still drc errror
            std::cout << "still drc error" << std::endl;
            bend_mode ^= true;
            track_path = track_path_known_good;
            assert(check_track_path(track_path));
            doc.b->get_board()->track_path = track_path;

            // create new segment
            // track_path.emplace_back(args.coords.x, args.coords.y);
            // track_path.emplace_back(args.coords.x, args.coords.y);
        }
        else {
            std::cout << "no drc error after flip" << std::endl;
            track_path_known_good = track_path;
            drc_okay = true;
        }
    }
    else {
        std::cout << "no drc error" << std::endl;
        track_path_known_good = track_path;
        drc_okay = true;
    }
    update_temp_track();
    return drc_okay;
}

void ToolRouteTrack::update_tip()
{
    std::stringstream ss;
    if (net) {
        ss << "<b>LMB:</b>place junction/connect <b>RMB:</b>finish and delete "
              "last segment <b>backspace:</b>delete last segment "
              "<b>v:</b>place via <b>/:</b>track posture <b>w:</b>change track "
              "width <b>W:</b>default track width  <b>Return:</b>save track "
              "<i>";
        if (net->name.size()) {
            ss << "routing net \"" << net->name << "\"";
        }
        else {
            ss << "routing unnamed net";
        }
        ss << "  track width " << dim_to_string(routing_width);
        if (routing_width_use_default) {
            ss << " (default)";
        }
        ss << "</i>";
    }
    else {
        ss << "<b>LMB:</b>select starting junction/pad <b>RMB:</b>cancel";
    }
    imp->tool_bar_set_tip(ss.str());
}

ToolResponse ToolRouteTrack::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::KEY) {
        if (args.key == GDK_KEY_Escape) {
            doc.b->get_board()->obstacles.clear();
            doc.b->get_board()->track_path.clear();
            return ToolResponse::revert();
        }
    }
    if (net == nullptr) { // begin route
        if (args.type == ToolEventType::CLICK) {
            if (args.target.type == ObjectType::PAD) {
                auto pkg = &doc.b->get_board()->packages.at(args.target.path.at(0));
                auto pad = &pkg->package.pads.at(args.target.path.at(1));
                net = pad->net;
                if (net) {
                    ToolArgs a;
                    a.type = args.type;
                    a.target = args.target;
                    a.coords = args.coords;
                    a.work_layer = args.work_layer;
                    if (!doc.b->get_board()->get_layers().at(a.work_layer).copper) {
                        a.work_layer = 0;
                    }
                    if (pad->padstack.type == Padstack::Type::TOP) {
                        a.work_layer = 0; // top
                    }
                    else if (pad->padstack.type == Padstack::Type::BOTTOM) {
                        a.work_layer = -100;
                    }
                    else if (pad->padstack.type == Padstack::Type::THROUGH) {
                        // it's okay
                    }

                    begin_track(a);
                    conn_start.connect(pkg, pad);
                    std::cout << "begin net" << std::endl;
                    update_tip();
                    imp->set_work_layer(a.work_layer);
                    return ToolResponse();
                }
                else {
                    imp->tool_bar_flash("pad is not connected to a net");
                }
            }
            else if (args.target.type == ObjectType::JUNCTION) {
                auto junc = doc.r->get_junction(args.target.path.at(0));
                net = junc->net;
                if (net) {
                    ToolArgs a;
                    a.type = args.type;
                    a.target = args.target;
                    a.coords = args.coords;
                    a.work_layer = args.work_layer;
                    if (!doc.b->get_board()->get_layers().at(a.work_layer).copper) {
                        a.work_layer = 0;
                    }
                    if (!junc->has_via) {
                        if (junc->layer < 10000) {
                            a.work_layer = junc->layer;
                        }
                    }
                    begin_track(a);
                    conn_start.connect(junc);
                    std::cout << "begin net" << std::endl;
                    update_tip();
                    imp->set_work_layer(a.work_layer);
                    return ToolResponse();
                }
                else {
                    imp->tool_bar_flash("junction is not connected to a net");
                }
            }
        }
    }
    else {
        if (args.type == ToolEventType::MOVE) {
            try_move_track(args);
            std::cout << "temp track" << std::endl;
            for (const auto &it : track_path) {
                std::cout << it.X << " " << it.Y << std::endl;
            }
            std::cout << std::endl << std::endl;
        }
        else if (args.type == ToolEventType::CLICK) {
            if (args.button == 1) {
                if (!args.target.is_valid()) {
                    track_path.emplace_back(args.coords.x, args.coords.y);
                    track_path.emplace_back(args.coords.x, args.coords.y);
                    bend_mode ^= true;
                    try_move_track(args);
                }
                else {
                    if (!try_move_track(args)) { // drc not okay
                        imp->tool_bar_flash("can't connect due to DRC violation");
                        return ToolResponse();
                    }
                    if (args.target.type == ObjectType::PAD) {
                        auto pkg = &doc.b->get_board()->packages.at(args.target.path.at(0));
                        auto pad = &pkg->package.pads.at(args.target.path.at(1));
                        if (pad->net != net) {
                            imp->tool_bar_flash("pad connected to wrong net");
                            return ToolResponse();
                        }
                        temp_tracks.back()->to.connect(pkg, pad);
                    }
                    else if (args.target.type == ObjectType::JUNCTION) {
                        std::cout << "temp track ju" << std::endl;
                        for (const auto &it : track_path) {
                            std::cout << it.X << " " << it.Y << std::endl;
                        }

                        auto junc = doc.r->get_junction(args.target.path.at(0));
                        if (junc->net && (junc->net != net)) {
                            imp->tool_bar_flash("junction connected to wrong net");
                            return ToolResponse();
                        }
                        temp_tracks.back()->to.connect(junc);
                    }
                    else {
                        return ToolResponse();
                    }

                    doc.r->delete_junction(temp_junctions.back()->uuid);

                    doc.b->get_board()->track_path.clear();
                    doc.b->get_board()->obstacles.clear();
                    return ToolResponse::commit();
                }
            }
            else if (args.button == 3) {
                auto brd = doc.b->get_board();

                if (track_path.size() >= 2 && via == nullptr) {
                    track_path.pop_back();
                    track_path.pop_back();
                    update_temp_track();
                }

                brd->track_path.clear();
                brd->obstacles.clear();
                return ToolResponse::commit();
            }
        }
        else if (args.type == ToolEventType::KEY) {
            if (args.key == GDK_KEY_slash) {
                bend_mode ^= true;
                auto &b = track_path.back();
                update_track({b.X, b.Y});
                if (!check_track_path(track_path)) { // drc error
                    bend_mode ^= true;
                    update_track({b.X, b.Y});
                }
                update_temp_track();
            }
            else if (args.key == GDK_KEY_v) {
                if (via == nullptr) {
                    auto vpp = doc.b->get_via_padstack_provider();
                    auto padstack = vpp->get_padstack(rules->get_via_padstack_uuid(net));
                    if (padstack) {
                        auto uu = UUID::random();
                        auto ps = rules->get_via_parameter_set(net);
                        via = &doc.b->get_board()
                                       ->vias
                                       .emplace(std::piecewise_construct, std::forward_as_tuple(uu),
                                                std::forward_as_tuple(uu, padstack))
                                       .first->second;
                        via->parameter_set = ps;
                        update_temp_track();
                    }
                }
                else {
                    doc.b->get_board()->vias.erase(via->uuid);
                    via = nullptr;
                }
            }
            else if (args.key == GDK_KEY_w || args.key == GDK_KEY_W) {
                auto use_default_before = routing_width_use_default;
                auto routing_width_before = routing_width;
                if (args.key == GDK_KEY_w) {
                    auto r = imp->dialogs.ask_datum("Track width", rules->get_default_track_width(net, routing_layer));
                    if (r.first) {
                        routing_width_use_default = false;
                        routing_width = r.second;
                    }
                    else {
                        return ToolResponse();
                    }
                }
                else {
                    routing_width_use_default = true;
                    routing_width = rules->get_default_track_width(net, routing_layer);
                }
                update_obstacles();
                if (!check_track_path(track_path)) { // true if no drc error
                    routing_width = routing_width_before;
                    routing_width_use_default = use_default_before;
                    imp->tool_bar_flash("Couldn't change width due to DRC error");
                    update_obstacles();
                }

                update_temp_track();
            }
            else if (args.key == GDK_KEY_Return) {
                if (temp_junctions.size() > 0) {
                    auto junc = temp_junctions.back();
                    temp_junctions.clear();
                    temp_tracks.clear();
                    begin_track(args);
                    conn_start.connect(junc);
                    update_tip();
                    update_temp_track();
                }
            }
            else if (args.key == GDK_KEY_BackSpace) {
                if (track_path.size() > 3) {
                    track_path.pop_back();
                    track_path.pop_back();
                    update_track(args.coords);
                    if (!check_track_path(track_path)) { // drc error
                        std::cout << "drc error" << std::endl;
                        bend_mode ^= true;
                        update_track(args.coords);           // flip
                        if (!check_track_path(track_path)) { // still drc errror
                            std::cout << "still drc error" << std::endl;
                            bend_mode ^= true;
                            track_path = track_path_known_good;
                            assert(check_track_path(track_path));
                            doc.b->get_board()->track_path = track_path;
                        }
                        else {
                            std::cout << "no drc error after flip" << std::endl;
                            track_path_known_good = track_path;
                        }
                    }
                    else {
                        std::cout << "no drc error" << std::endl;
                        track_path_known_good = track_path;
                    }
                    update_temp_track();
                }
            }
        }
    }
    update_tip();
    return ToolResponse();
}
} // namespace horizon
