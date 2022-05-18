#include "tool_move_track_center.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include "util/selection_util.hpp"
#include "imp/imp_interface.hpp"

namespace horizon {

std::set<class Track *> ToolMoveTrackCenter::get_tracks()
{
    std::set<Track *> trs;
    for (const auto &it : selection) {
        if (it.type == ObjectType::TRACK) {
            auto &track = doc.b->get_board()->tracks.at(it.uuid);
            if (track.is_arc())
                trs.insert(&track);
        }
    }
    return trs;
}

bool ToolMoveTrackCenter::can_begin()
{
    return get_tracks().size();
}

ToolResponse ToolMoveTrackCenter::begin(const ToolArgs &args)
{
    tracks = get_tracks();
    selection.clear();
    for (auto tr : tracks) {
        selection.emplace(tr->uuid, ObjectType::TRACK);
    }
    last = args.coords;
    imp->tool_bar_set_actions({
            {InToolActionID::LMB, "finish"},
            {InToolActionID::RMB},
    });
    return {};
}

ToolResponse ToolMoveTrackCenter::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        auto delta = args.coords - last;
        for (auto tr : tracks) {
            tr->center.value() += delta;
        }
        last = args.coords;
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB:
            return ToolResponse::commit();

        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            return ToolResponse::revert();
        default:;
        }
    }
    return {};
}

} // namespace horizon
