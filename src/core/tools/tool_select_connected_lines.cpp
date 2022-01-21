#include "tool_select_connected_lines.hpp"
#include "canvas/canvas_gl.hpp"
#include "common/line.hpp"
#include "imp/imp_interface.hpp"
#include "util/selection_util.hpp"
#include "util/uuid.hpp"
#include <functional>
#include <vector>
#include <unordered_set>
#include "document/idocument.hpp"

namespace horizon {

bool ToolSelectConnectedLines::can_begin()
{
    if (!doc.r->has_object_type(ObjectType::LINE))
        return false;
    return sel_has_type(selection, ObjectType::LINE);
}

ToolResponse ToolSelectConnectedLines::begin(const ToolArgs &args)
{
    auto sel_line_uuid = sel_find_one(selection, ObjectType::LINE).uuid;

    std::unordered_set<UUID> visited;
    std::vector<UUID> to_visit = {sel_line_uuid};

    while (!to_visit.empty()) {
        if (visited.count(to_visit.back()) != 0) {
            to_visit.pop_back();
            continue;
        }
        auto *ln = doc.r->get_line(to_visit.back());
        to_visit.pop_back();
        visited.insert(ln->uuid);
        selection.emplace(ln->uuid, ObjectType::LINE);

        auto *from_junc = doc.r->get_junction(ln->from.uuid);
        auto *to_junc = doc.r->get_junction(ln->to.uuid);

        std::copy(from_junc->connected_lines.begin(), from_junc->connected_lines.end(), std::back_inserter(to_visit));
        std::copy(to_junc->connected_lines.begin(), to_junc->connected_lines.end(), std::back_inserter(to_visit));
    }

    if (visited.size() == 1)
        imp->tool_bar_flash("no connected lines found");
    else
        imp->get_canvas()->set_selection_mode(CanvasGL::SelectionMode::NORMAL);

    return ToolResponse::end();
}

ToolResponse ToolSelectConnectedLines::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
