#include "tool_place_junction.hpp"
#include "document/idocument_schematic.hpp"
#include "schematic/schematic.hpp"
#include "document/idocument_symbol.hpp"
#include "pool/symbol.hpp"
#include "imp/imp_interface.hpp"
#include <gdk/gdkkeysyms.h>
#include <iostream>

namespace horizon {

ToolPlaceJunction::ToolPlaceJunction(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

bool ToolPlaceJunction::can_begin()
{
    return doc.r->has_object_type(ObjectType::JUNCTION);
}

ToolResponse ToolPlaceJunction::begin(const ToolArgs &args)
{
    std::cout << "tool place junction\n";

    if (!begin_attached()) {
        return ToolResponse::end();
    }

    create_junction(args.coords);
    create_attached();

    return ToolResponse();
}

void ToolPlaceJunction::create_junction(const Coordi &c)
{
    temp = doc.r->insert_junction(UUID::random());
    imp->set_snap_filter({{ObjectType::JUNCTION, temp->uuid}});
    temp->position = c;
}

ToolResponse ToolPlaceJunction::update(const ToolArgs &args)
{
    if (update_attached(args)) {
        return ToolResponse::fast();
    }

    if (args.type == ToolEventType::MOVE) {
        temp->position = args.coords;
        return ToolResponse::fast();
    }
    else if (args.type == ToolEventType::CLICK) {
        if (args.button == 1) {
            if (doc.c) {
                for (auto it : doc.c->get_net_lines()) {
                    if (it->coord_on_line(temp->position)) {
                        std::cout << "on line" << std::endl;
                        if (!check_line(it))
                            return ToolResponse();

                        doc.c->get_sheet()->split_line_net(it, temp);
                        break;
                    }
                }
            }
            junctions_placed.push_front(temp);

            create_junction(args.coords);
            create_attached();
        }
        else if (args.button == 3) {
            delete_attached();
            doc.r->delete_junction(temp->uuid);
            temp = 0;
            selection.clear();
            for (auto it : junctions_placed) {
                selection.emplace(it->uuid, ObjectType::JUNCTION);
            }
            return ToolResponse::commit();
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
