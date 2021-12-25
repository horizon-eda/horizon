#include "tool_merge_duplicate_junctions.hpp"
#include "common/line.hpp"
#include "common/arc.hpp"
#include "common/object_descr.hpp"
#include "imp/imp_interface.hpp"
#include "util/geom_util.hpp"
#include "util/selection_util.hpp"
#include "util/uuid.hpp"
#include <iostream>
#include <functional>
#include <cmath>
#include <vector>
#include <map>
#include "document/idocument.hpp"

namespace horizon {

bool ToolMergeDuplicateJunctions::can_begin()
{
    if (!(doc.r->has_object_type(ObjectType::JUNCTION)))
        return false;
    return sel_count_type(selection, ObjectType::JUNCTION) >= 2;
}

ToolResponse ToolMergeDuplicateJunctions::begin(const ToolArgs &args)
{
    std::map<Coordi, std::vector<UUID>> juncs;

    for (const auto &it : selection) {
        if (it.type != ObjectType::JUNCTION) {
            continue;
        }
        auto *junc = doc.r->get_junction(it.uuid);
        if (!junc->only_lines_arcs_connected()) {
            imp->tool_bar_flash("cannot merge junctions that are connected to more than just lines/arcs");
            return ToolResponse::revert();
        }
        Coordi rounded(junc->position.x / 0.01_mm, junc->position.y / 0.01_mm);
        juncs[rounded].push_back(junc->uuid);
    }

    size_t total_before = 0, total_after = 0;

    for (auto &[pos, equal_juncs] : juncs) {
        if (equal_juncs.size() < 2) {
            continue;
        }
        std::cout << "merging " << equal_juncs.size() << " junctions at " << coord_to_string(pos) << std::endl;
        total_after++;
        total_before += equal_juncs.size();
        auto result_uuid = equal_juncs.back();
        auto *result_ptr = doc.r->get_junction(result_uuid);
        equal_juncs.pop_back();
        for (const auto &junc_uuid : equal_juncs) {
            auto *junc = doc.r->get_junction(junc_uuid);
            for (const auto &ln_uuid : junc->connected_lines) {
                auto *ln = doc.r->get_line(ln_uuid);
                if (ln->from == junc) {
                    ln->from = result_ptr;
                }
                if (ln->to == junc) {
                    ln->to = result_ptr;
                }
            }
            for (const auto &arc_uuid : junc->connected_arcs) {
                auto *arc = doc.r->get_arc(arc_uuid);
                if (arc->from == junc) {
                    arc->from = result_ptr;
                }
                if (arc->center == junc) {
                    arc->center = result_ptr;
                }
                if (arc->to == junc) {
                    arc->to = result_ptr;
                }
            }
            doc.r->delete_junction(junc_uuid);
        }
        // TODO: proper BoardJunction/SchematicJunction support (more than lines & arcs)
    }

    if (total_before == total_after) {
        imp->tool_bar_flash("no mergeable junctions found");
        return ToolResponse::end();
    }

    imp->tool_bar_flash("merged " + std::to_string(total_before) + " "
                        + object_descriptions.at(ObjectType::JUNCTION).get_name_for_n(total_before) + " into "
                        + std::to_string(total_after));
    return ToolResponse::commit();
}

ToolResponse ToolMergeDuplicateJunctions::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
