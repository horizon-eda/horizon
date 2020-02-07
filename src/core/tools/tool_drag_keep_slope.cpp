#include "tool_drag_keep_slope.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include <iostream>
#include <gdk/gdkkeysyms.h>

namespace horizon {

ToolDragKeepSlope::ToolDragKeepSlope(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

bool ToolDragKeepSlope::can_begin()
{
    if (!doc.b)
        return false;
    for (const auto &it : selection) {
        if (it.type == ObjectType::TRACK)
            return true;
    }
    return false;
}

ToolDragKeepSlope::TrackInfo::TrackInfo(Track *tr, Track *fr, Track *to) : track(tr), track_from(fr), track_to(to)
{
    if (track_from->from.junc == track->from.junc) {
        pos_from2 = track_from->to.get_position();
    }
    else {
        pos_from2 = track_from->from.get_position();
    }
    if (track_to->from.junc == track->to.junc) {
        pos_to2 = track_to->to.get_position();
    }
    else {
        pos_to2 = track_to->from.get_position();
    }
    pos_from_orig = tr->from.get_position();
    pos_to_orig = tr->to.get_position();
}

ToolResponse ToolDragKeepSlope::begin(const ToolArgs &args)
{
    std::cout << "tool drag keep slope\n";

    selection.clear();
    for (const auto &it : args.selection) {
        if (it.type == ObjectType::TRACK) {
            auto track = &doc.b->get_board()->tracks.at(it.uuid);
            if (track->from.is_junc() && track->to.is_junc()) {
                if (track->from.junc->connection_count == 2 && track->to.junc->connection_count == 2) {
                    Track *tr_from = nullptr;
                    Track *tr_to = nullptr;
                    for (auto &it_tr : doc.b->get_board()->tracks) {
                        auto track_other = &it_tr.second;
                        if (track_other == track)
                            continue;
                        for (const auto &it_ft : {track_other->from, track_other->to}) {
                            if (it_ft.junc == track->from.junc) {
                                assert(tr_from == nullptr);
                                tr_from = track_other;
                            }
                            if (it_ft.junc == track->to.junc) {
                                assert(tr_to == nullptr);
                                tr_to = track_other;
                            }
                        }
                    }
                    assert(tr_from);
                    assert(tr_to);
                    if ((tr_from->from.get_position() != tr_from->to.get_position())
                        && (tr_to->from.get_position() != tr_to->to.get_position())) {
                        track_info.emplace_back(track, tr_from, tr_to);
                        selection.emplace(it);
                        selection.emplace(track->from.junc->uuid, ObjectType::JUNCTION);
                        selection.emplace(track->to.junc->uuid, ObjectType::JUNCTION);
                    }
                }
            }
        }
    }
    pos_orig = args.coords;

    if (selection.size() < 1)
        return ToolResponse::end();

    return ToolResponse();
}

typedef Coord<double> Coordd;

ToolResponse ToolDragKeepSlope::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        for (const auto &it : track_info) {
            Coordd vfrom = it.pos_from_orig - it.pos_from2;
            Coordd vto = it.pos_to_orig - it.pos_to2;
            Coordd vtr = it.pos_to_orig - it.pos_from_orig;
            Coordd vtrn(vtr.y, -vtr.x);

            Coordd vshift = args.coords - pos_orig;
            Coordd vshift2 = (vtrn * (vtrn.dot(vshift))) / vtrn.mag_sq();

            Coordd shift_from = (vfrom * vshift2.mag_sq()) / (vfrom.dot(vshift2));
            Coordd shift_to = (vto * vshift2.mag_sq()) / (vto.dot(vshift2));
            if (vshift2.mag_sq() == 0) {
                shift_from = {0, 0};
                shift_to = {0, 0};
            }

            it.track->from.junc->position = it.pos_from_orig + Coordi(shift_from.x, shift_from.y);
            it.track->to.junc->position = it.pos_to_orig + Coordi(shift_to.x, shift_to.y);
        }
    }
    else if (args.type == ToolEventType::CLICK) {
        if (args.button == 1) {
            return ToolResponse::commit();
        }
        else if (args.button == 3) {
            return ToolResponse::revert();
        }
    }

    else if (args.type == ToolEventType::KEY) {
        if (args.key == GDK_KEY_Escape) {
            return ToolResponse::revert();
        }
    }
    return ToolResponse();
}
} // namespace horizon
