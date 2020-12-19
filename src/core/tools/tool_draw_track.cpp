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

    const auto uu = UUID::random();
    temp_junc = &doc.b->get_board()->junctions.emplace(uu, uu).first->second;
    imp->set_snap_filter({{ObjectType::JUNCTION, temp_junc->uuid}});
    temp_junc->position = args.coords;
    temp_track = nullptr;
    selection.clear();

    imp->tool_bar_set_actions({
            {InToolActionID::LMB},
            {InToolActionID::RMB, "finish"},
    });

    rules = dynamic_cast<BoardRules *>(doc.r->get_rules());
    return ToolResponse();
}

Track *ToolDrawTrack::create_temp_track()
{
    auto uu = UUID::random();
    temp_track = &doc.b->get_board()->tracks.emplace(uu, uu).first->second;
    return temp_track;
}

ToolResponse ToolDrawTrack::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        temp_junc->position = args.coords;

        if (temp_junc->net) {
            std::set<UUID> nets;
            nets.insert(temp_junc->net->uuid);
            doc.b->get_board()->update_airwires(true, nets);
        }
        return ToolResponse();
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB:
            if (args.target.type == ObjectType::JUNCTION) {
                auto &j = doc.b->get_board()->junctions.at(args.target.path.at(0));
                if (temp_track != nullptr) {
                    if (temp_track->net && j.net && (temp_track->net->uuid != j.net->uuid)) {
                        return ToolResponse();
                    }
                    temp_track->to.connect(&j);
                    if (temp_track->net) {
                        j.net = temp_track->net;
                    }
                    else {
                        temp_track->net = j.net;
                    }
                }

                create_temp_track();
                temp_junc->net = j.net;
                temp_track->from.connect(&j);
                temp_track->to.connect(temp_junc);
                temp_track->net = j.net;
                temp_track->width = rules->get_default_track_width(j.net, 0);
            }
            else if (args.target.type == ObjectType::PAD) {
                auto pkg = &doc.b->get_board()->packages.at(args.target.path.at(0));
                auto pad = &pkg->package.pads.at(args.target.path.at(1));
                if (temp_track != nullptr) {
                    if (temp_track->net && (temp_track->net->uuid != pad->net->uuid)) {
                        return ToolResponse();
                    }
                    temp_track->to.connect(pkg, pad);
                    temp_track->net = pad->net;
                }
                create_temp_track();
                temp_track->from.connect(pkg, &pkg->package.pads.at(args.target.path.at(1)));
                temp_track->to.connect(temp_junc);
                temp_track->net = pad->net;
                temp_junc->net = pad->net;
                if (pad->net) {
                    temp_track->width = rules->get_default_track_width(pad->net, 0);
                }
            }

            else {
                BoardJunction *last = temp_junc;
                const auto uu = UUID::random();
                temp_junc = &doc.b->get_board()->junctions.emplace(uu, uu).first->second;
                imp->set_snap_filter({{ObjectType::JUNCTION, temp_junc->uuid}});
                temp_junc->position = args.coords;
                if (last && temp_track) {
                    temp_track->net = last->net;
                }
                if (temp_track) {
                    temp_junc->net = temp_track->net;
                }
                if (last) {
                    temp_junc->net = last->net;
                }

                create_temp_track();
                temp_track->from.connect(last);
                temp_track->to.connect(temp_junc);
                temp_track->net = last->net;
                if (last->net) {
                    temp_track->width = rules->get_default_track_width(last->net, 0);
                }
            }
            break;

        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            if (temp_track) {
                doc.b->get_board()->tracks.erase(temp_track->uuid);
                temp_track = nullptr;
            }
            doc.b->delete_junction(temp_junc->uuid);
            temp_junc = nullptr;
            return ToolResponse::commit();

        default:;
        }
    }
    return ToolResponse();
}
} // namespace horizon
