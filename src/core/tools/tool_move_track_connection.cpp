#include "tool_move_track_connection.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include "util/selection_util.hpp"
#include "imp/imp_interface.hpp"
#include "canvas/canvas_pads.hpp"

namespace horizon {

bool ToolMoveTrackConnection::can_begin()
{
    auto sel = sel_find_exactly_one(selection, ObjectType::TRACK);
    if (!sel)
        return false;
    const auto &tr = doc.b->get_board()->tracks.at(sel->uuid);
    if (tr.from.is_pad() || tr.to.is_pad())
        return true;
    else
        return false;
}

ToolResponse ToolMoveTrackConnection::begin(const ToolArgs &args)
{
    auto &brd = *doc.b->get_board();
    track = &brd.tracks.at(sel_find_one(selection, ObjectType::TRACK).uuid);
    if (track->from.is_junc())
        conn = &track->to;
    else if (track->to.is_junc())
        conn = &track->from;
    if (!conn) {
        imp->tool_bar_set_actions({
                {InToolActionID::LMB, "pick pad"},
                {InToolActionID::RMB},
        });
    }
    else {
        imp->tool_bar_set_actions({
                {InToolActionID::LMB, "finish"},
                {InToolActionID::RMB},
        });
        move(args.coords);
    }
    return ToolResponse();
}
ToolResponse ToolMoveTrackConnection::update(const ToolArgs &args)
{
    if (!conn) {
        if (args.type == ToolEventType::ACTION) {
            switch (args.action) {
            case InToolActionID::LMB: {
                if (args.target.type == ObjectType::PAD || args.target.type == ObjectType::BOARD_PACKAGE) {
                    auto pkgpad = pad_from_target(args.target);
                    if (!pkgpad) {
                        imp->tool_bar_flash("please pick a pad");
                        return ToolResponse();
                    }
                    if (track->to.is_pad() && track->to.package->uuid == pkgpad->pkg.uuid
                        && track->to.pad->uuid == pkgpad->pad.uuid) {
                        conn = &track->to;
                    }
                    else if (track->from.is_pad() && track->from.package->uuid == pkgpad->pkg.uuid
                             && track->from.pad->uuid == pkgpad->pad.uuid) {
                        conn = &track->from;
                    }
                    else {
                        imp->tool_bar_flash("pick a pad connected to the selected track");
                    }
                    if (conn) {
                        move(args.coords);
                        imp->tool_bar_set_actions({
                                {InToolActionID::LMB, "finish"},
                                {InToolActionID::RMB},
                        });
                    }
                }
                else {
                    imp->tool_bar_flash("please pick a pad");
                }
            } break;

            case InToolActionID::RMB:
            case InToolActionID::CANCEL:
                return ToolResponse::end();
            default:;
            }
        }
    }
    else {
        if (args.type == ToolEventType::ACTION) {
            switch (args.action) {
            case InToolActionID::LMB: {
                if (conn->offset == Coordi())
                    return ToolResponse::commit();
                CanvasPads ca_pads;
                ca_pads.update(*doc.b->get_board());
                const CanvasPads::PadKey key{track->layer, conn->package->uuid, conn->pad->uuid};
                if (ca_pads.pads.count(key)) {
                    auto &[placement, paths] = ca_pads.pads.at(key);
                    const auto p = conn->get_position();
                    for (const auto &path : paths) {
                        if (ClipperLib::PointInPolygon({p.x, p.y}, path))
                            return ToolResponse::commit();
                    }
                    imp->tool_bar_flash("track not connected to pad");
                }
                else {
                    imp->tool_bar_flash("pad not found");
                }
            } break;

            case InToolActionID::RMB:
            case InToolActionID::CANCEL:
                return ToolResponse::revert();
            default:;
            }
        }
        else if (args.type == ToolEventType::MOVE) {
            move(args.coords);
        }
    }
    return ToolResponse();
}

void ToolMoveTrackConnection::move(const Coordi &c)
{
    auto tr = conn->package->placement;
    if (conn->package->flip)
        tr.invert_angle();

    Placement p{c};
    p.make_relative(tr);

    conn->offset = p.shift - conn->pad->placement.shift;
}

} // namespace horizon
