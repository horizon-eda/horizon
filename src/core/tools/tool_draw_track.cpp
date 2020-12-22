#include "tool_draw_track.hpp"
#include "board/board_rules.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include "imp/imp_interface.hpp"
#include <iostream>

namespace horizon {

bool ToolDrawTrack::can_begin()
{
    return doc.b;
}

ToolResponse ToolDrawTrack::begin(const ToolArgs &args)
{
    std::cout << "tool draw track\n";

    /*const auto uu = UUID::random();
    temp_junc = &doc.b->get_board()->junctions.emplace(uu, uu).first->second;
    imp->set_snap_filter({{ObjectType::JUNCTION, temp_junc->uuid}});
    temp_junc->position = args.coords;
    temp_track = nullptr;*/
    selection.clear();

    imp->tool_bar_set_actions({
            {InToolActionID::LMB},
            {InToolActionID::RMB, "finish"},
    });

    rules = dynamic_cast<BoardRules *>(doc.r->get_rules());
    return ToolResponse();
}

void ToolDrawTrack::create_temp_track()
{
    auto uu = UUID::random();
    temp_track = &doc.b->get_board()->tracks.emplace(uu, uu).first->second;
    temp_junc = &doc.b->get_board()->junctions.emplace(uu, uu).first->second;
    temp_track->to.connect(temp_junc);
    imp->set_snap_filter({{ObjectType::JUNCTION, temp_junc->uuid}});
}

ToolResponse ToolDrawTrack::update(const ToolArgs &args)
{
    auto &brd = *doc.b->get_board();
    if (args.type == ToolEventType::MOVE) {
        if (temp_junc) {
            temp_junc->position = args.coords;
            if (temp_junc->net) {
                std::set<UUID> nets;
                nets.insert(temp_junc->net->uuid);
                brd.update_airwires(true, nets);
            }
        }
        return ToolResponse();
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB:
            if (temp_junc) {
                if (args.target.type == ObjectType::JUNCTION) {
                    auto &ju = brd.junctions.at(args.target.path.at(0));
                    if (ju.net == nullptr && temp_junc->net == nullptr) {
                        imp->tool_bar_flash("can't connect no net to no net");
                    }
                    else if ((ju.net && temp_junc->net == nullptr) || (ju.net == nullptr && temp_junc->net)
                             || (ju.net == temp_junc->net)) {
                        temp_track->to.connect(&ju);
                        if (ju.net)
                            brd.airwires_expand = {ju.net->uuid};
                        else if (temp_junc->net)
                            brd.airwires_expand = {temp_junc->net->uuid};
                        brd.junctions.erase(temp_junc->uuid);
                        temp_junc = nullptr;
                        brd.expand_flags = Board::EXPAND_PROPAGATE_NETS | Board::EXPAND_AIRWIRES;
                        return ToolResponse::commit();
                    }
                    else if (ju.net != temp_junc->net) {
                        imp->tool_bar_flash("can't connect different nets");
                    }
                }
                else if (args.target.type == ObjectType::PAD) {
                    auto &pkg = brd.packages.at(args.target.path.at(0));
                    auto &pad = pkg.package.pads.at(args.target.path.at(1));
                    if (pad.net) {
                        // if (temp_junc->net && (temp_junc->net != pad.net)) {
                        if ((temp_junc->net == pad.net) || (temp_junc->net == nullptr)) {
                            temp_track->to.connect(&pkg, &pad);
                            brd.junctions.erase(temp_junc->uuid);
                            temp_junc = nullptr;
                            brd.airwires_expand = {pad.net->uuid};
                            brd.expand_flags = Board::EXPAND_PROPAGATE_NETS | Board::EXPAND_AIRWIRES;
                            return ToolResponse::commit();
                        }
                        else {
                            imp->tool_bar_flash("can't connect different nets");
                        }
                    }
                    else {
                        imp->tool_bar_flash("can't connect to pad without net");
                    }
                }
                else if (args.target.type == ObjectType::INVALID) {
                    if (temp_junc->net) {
                        brd.airwires_expand = {temp_junc->net->uuid};
                        brd.expand_flags = Board::EXPAND_PROPAGATE_NETS | Board::EXPAND_AIRWIRES;
                        return ToolResponse::commit();
                    }
                    else {
                        imp->tool_bar_flash("can't connect no-net track to nowhere");
                    }
                }
                else {
                    imp->tool_bar_flash("unsupported target");
                }
            }
            else {
                // start
                if (args.target.type == ObjectType::JUNCTION) {
                    create_temp_track();
                    auto &ju = brd.junctions.at(args.target.path.at(0));
                    temp_track->from.connect(&ju);
                    temp_track->layer = ju.layer.start();
                    temp_track->net = ju.net;
                    temp_junc->layer = temp_track->layer;
                    temp_junc->net = ju.net;
                    temp_junc->position = args.coords;
                    temp_track->width = 0.1_mm;
                    for (const auto &it : ju.connected_tracks) {
                        const auto &tr = brd.tracks.at(it);
                        temp_track->width = tr.width;
                        break;
                    }
                }
                else if (args.target.type == ObjectType::PAD) {
                    auto &pkg = brd.packages.at(args.target.path.at(0));
                    auto &pad = pkg.package.pads.at(args.target.path.at(1));
                    if (pad.net) {
                        create_temp_track();
                        temp_track->from.connect(&pkg, &pad);
                        temp_track->layer = args.work_layer;
                        temp_track->net = pad.net;
                        temp_junc->layer = args.work_layer;
                        temp_junc->net = pad.net;
                        temp_track->width = rules->get_default_track_width(temp_track->net, temp_track->layer);
                        temp_junc->position = args.coords;
                    }
                    else {
                        imp->tool_bar_flash("can't connect to pad without net");
                    }
                }
            }
            break;

        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            return ToolResponse::revert();

        default:;
        }
    }
    return ToolResponse();
}
} // namespace horizon
