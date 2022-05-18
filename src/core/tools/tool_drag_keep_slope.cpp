#include "tool_drag_keep_slope.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include "imp/imp_interface.hpp"
#include "util/selection_util.hpp"

namespace horizon {

bool ToolDragKeepSlope::can_begin()
{
    if (!doc.b)
        return false;
    return sel_has_type(selection, ObjectType::TRACK);
}

ToolDragKeepSlope::TrackInfo::TrackInfo(Track &tr, const Track &track_from, const Track &track_to) : track(tr)
{
    if (track_from.from.junc == track.from.junc) {
        pos_from2 = track_from.to.get_position();
    }
    else {
        pos_from2 = track_from.from.get_position();
    }
    if (track_to.from.junc == track.to.junc) {
        pos_to2 = track_to.to.get_position();
    }
    else {
        pos_to2 = track_to.from.get_position();
    }
    pos_from_orig = tr.from.get_position();
    pos_to_orig = tr.to.get_position();
    if (tr.is_arc())
        center_orig = tr.center.value();
}

ToolResponse ToolDragKeepSlope::begin(const ToolArgs &args)
{
    selection.clear();
    for (const auto &it : args.selection) {
        if (it.type == ObjectType::TRACK) {
            auto track = &doc.b->get_board()->tracks.at(it.uuid);
            if (track->from.is_junc() && track->to.is_junc()) {
                if (track->from.junc->connected_tracks.size() == 2 && track->to.junc->connected_tracks.size() == 2) {
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
                        track_info.emplace_back(*track, *tr_from, *tr_to);
                        selection.emplace(it);
                        selection.emplace(track->from.junc->uuid, ObjectType::JUNCTION);
                        selection.emplace(track->to.junc->uuid, ObjectType::JUNCTION);
                    }
                }
            }
        }
    }
    pos_orig = args.coords;

    for (const auto &it : track_info) {
        nets.insert(it.track.net->uuid);
    }

    if (selection.size() < 1)
        return ToolResponse::end();

    imp->tool_bar_set_actions({
            {InToolActionID::LMB, "place"},
            {InToolActionID::RMB, "cancel"},
    });

    return ToolResponse();
}

ToolResponse ToolDragKeepSlope::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        for (const auto &it : track_info) {
            const auto pos = it.get_pos(args.coords - pos_orig);
            it.track.from.junc->position = pos.from;
            it.track.to.junc->position = pos.to;
            if (it.track.is_arc()) {
                it.track.center = it.center_orig + pos.arc_center;
            }
        }
        doc.b->get_board()->update_airwires(true, nets);
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB: {
            doc.b->get_board()->update_airwires(false, nets);
            return ToolResponse::commit();
        }

        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            return ToolResponse::revert();

        default:;
        }
    }
    return ToolResponse();
}
} // namespace horizon
