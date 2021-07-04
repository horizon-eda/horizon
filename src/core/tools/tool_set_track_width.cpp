#include "tool_set_track_width.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include "util/selection_util.hpp"
#include "imp/imp_interface.hpp"

namespace horizon {

bool ToolSetTrackWidth::can_begin()
{
    return sel_count_type(selection, ObjectType::TRACK);
}

ToolResponse ToolSetTrackWidth::begin(const ToolArgs &args)
{
    auto &brd = *doc.b->get_board();
    uint64_t width = 0;
    for (const auto &it : args.selection) {
        if (it.type == ObjectType::TRACK) {
            const auto &tr = brd.tracks.at(it.uuid);
            width = std::max(width, tr.width);
        }
    }

    if (const auto w = imp->dialogs.ask_datum("Enter width", width)) {
        for (const auto &it : args.selection) {
            if (it.type == ObjectType::TRACK) {
                auto &tr = brd.tracks.at(it.uuid);
                tr.width = *w;
                tr.width_from_rules = false;
            }
        }
        return ToolResponse::commit();
    }
    else {
        return ToolResponse::end();
    }
}
ToolResponse ToolSetTrackWidth::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
