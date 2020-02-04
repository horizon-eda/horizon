#include "tool_place_shape.hpp"
#include "core_padstack.hpp"
#include "imp/imp_interface.hpp"
#include <iostream>

namespace horizon {

ToolPlaceShape::ToolPlaceShape(Core *c, ToolID tid) : ToolBase(c, tid)
{
}

bool ToolPlaceShape::can_begin()
{
    return core.a;
}

ToolResponse ToolPlaceShape::begin(const ToolArgs &args)
{
    std::cout << "tool place shape\n";

    create_shape(args.coords);
    temp->layer = args.work_layer;

    imp->tool_bar_set_tip(
            "<b>LMB:</b>place shape <b>RMB:</b>delete current shape and "
            "finish <b>i:</b>edit shape <b>r:</b>rotate");
    return ToolResponse();
}

void ToolPlaceShape::create_shape(const Coordi &c)
{
    auto uu = UUID::random();
    temp = &core.a->get_padstack()->shapes.emplace(uu, uu).first->second;
    temp->placement.shift = c;
}

ToolResponse ToolPlaceShape::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        temp->placement.shift = args.coords;
    }
    else if (args.type == ToolEventType::CLICK) {
        if (args.button == 1) {
            auto last = temp;
            shapes_placed.push_front(temp);

            create_shape(args.coords);
            temp->layer = last->layer;
            temp->form = last->form;
            temp->params = last->params;
        }
        else if (args.button == 3) {
            core.a->get_padstack()->shapes.erase(temp->uuid);
            temp = 0;
            core.r->commit();
            selection.clear();
            for (auto it : shapes_placed) {
                selection.emplace(it->uuid, ObjectType::SHAPE);
            }
            return ToolResponse::end();
        }
    }
    else if (args.type == ToolEventType::LAYER_CHANGE) {
        temp->layer = args.work_layer;
    }
    else if (args.type == ToolEventType::KEY) {
        if (args.key == GDK_KEY_Escape) {
            core.r->revert();
            return ToolResponse::end();
        }
        else if (args.key == GDK_KEY_i) {
            auto form = temp->form;
            auto params = temp->params;
            if (imp->dialogs.edit_shapes({temp}) == false) { // rollback
                temp->form = form;
                temp->params = params;
            }
        }
        else if (args.key == GDK_KEY_r) {
            temp->placement.inc_angle_deg(90);
        }
    }
    return ToolResponse();
}
} // namespace horizon
