#include "tool_place_junction_schematic.hpp"
#include "document/idocument_schematic.hpp"
#include "schematic/sheet.hpp"

namespace horizon {
bool ToolPlaceJunctionSchematic::junction_placed()
{
    for (auto &[uu, it] : doc.c->get_sheet()->net_lines) {
        if (it.coord_on_line(get_junction()->position)) {
            if (!check_line(&it))
                return true;

            doc.c->get_sheet()->split_line_net(&it, temp);
            break;
        }
    }
    return false;
}

void ToolPlaceJunctionSchematic::insert_junction()
{
    const auto uu = UUID::random();
    temp = &doc.c->get_sheet()->junctions.emplace(uu, uu).first->second;
}

} // namespace horizon
