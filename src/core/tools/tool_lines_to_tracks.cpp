#include "tool_lines_to_tracks.hpp"
#include "board/board.hpp"
#include "board/board_layers.hpp"
#include "common/line.hpp"
#include "imp/imp_interface.hpp"
#include "util/selection_util.hpp"
#include "util/uuid.hpp"
#include "document/idocument_board.hpp"

namespace horizon {

bool ToolLinesToTracks::can_begin()
{
    if (!doc.b)
        return false;
    return sel_has_type(selection, ObjectType::LINE);
}

ToolResponse ToolLinesToTracks::begin(const ToolArgs &args)
{
    auto &brd = *doc.b->get_board();

    for (const auto &it : selection) {
        if (it.type != ObjectType::LINE)
            continue;

        auto *ln = doc.r->get_line(it.uuid);
        if (!BoardLayers::is_copper(ln->layer)) {
            imp->tool_bar_flash("cannot convert lines on non-copper layers to tracks");
            return ToolResponse::revert();
        }

        auto uu = UUID::random();
        auto *track = &brd.tracks.emplace(uu, uu).first->second;
        auto *from_junc = &brd.junctions.at(ln->from.uuid);
        auto *to_junc = &brd.junctions.at(ln->to.uuid);
        if (!from_junc->only_lines_arcs_connected() || !to_junc->only_lines_arcs_connected()) {
            // lots of potential to e.g. create tracks between different nets
            imp->tool_bar_flash(
                    "cannot convert a line that is already connected to board elements (not just lines/arcs)");
            return ToolResponse::revert();
        }
        track->to.connect(to_junc);
        track->from.connect(from_junc);
        track->width_from_rules = false;
        track->width = ln->width;
        track->layer = ln->layer;
        doc.r->delete_line(ln->uuid);
    }

    return ToolResponse::commit();
}

ToolResponse ToolLinesToTracks::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
