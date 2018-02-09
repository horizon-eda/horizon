#include "tool_place_text.hpp"
#include "imp/imp_interface.hpp"
#include <iostream>

namespace horizon {

ToolPlaceText::ToolPlaceText(Core *c, ToolID tid) : ToolBase(c, tid)
{
}

bool ToolPlaceText::can_begin()
{
    return core.r->has_object_type(ObjectType::TEXT);
}

ToolResponse ToolPlaceText::begin(const ToolArgs &args)
{
    std::cout << "tool place text\n";

    temp = core.r->insert_text(UUID::random());
    temp->layer = args.work_layer;
    temp->placement.shift = args.coords;
    imp->tool_bar_set_tip(
            "<b>LMB:</b>place text <b>RMB:</b>finish "
            "<b>space:</b>change text");

    auto r = imp->dialogs.ask_datum_string("Enter text", temp->text);
    if (r.first) {
        temp->text = r.second;
    }
    else {
        core.r->delete_text(temp->uuid);
        core.r->selection.clear();
        return ToolResponse::end();
    }

    return ToolResponse();
}
ToolResponse ToolPlaceText::update(const ToolArgs &args)
{
    std::string *text;

    if (args.type == ToolEventType::MOVE) {
        temp->placement.shift = args.coords;
    }
    else if (args.type == ToolEventType::CLICK) {
        if (args.button == 1) {
            text = &temp->text;
            texts_placed.push_front(temp);
            temp = core.r->insert_text(UUID::random());
            temp->text = *text;
            temp->layer = args.work_layer;
            temp->placement.shift = args.coords;
        }
        else if (args.button == 3) {
            core.r->delete_text(temp->uuid);
            core.r->selection.clear();
            for (auto it : texts_placed) {
                core.r->selection.emplace(it->uuid, ObjectType::TEXT);
            }
            core.r->commit();
            return ToolResponse::end();
        }
    }
    else if (args.type == ToolEventType::LAYER_CHANGE) {
        temp->layer = args.work_layer;
    }
    else if (args.type == ToolEventType::KEY) {
        if (args.key == GDK_KEY_space) {
            auto r = imp->dialogs.ask_datum_string("Enter text", temp->text);
            if (r.first) {
                temp->text = r.second;
            }
        }
        else if (args.key == GDK_KEY_Escape) {
            core.r->revert();
            return ToolResponse::end();
        }
    }
    return ToolResponse();
}
} // namespace horizon
