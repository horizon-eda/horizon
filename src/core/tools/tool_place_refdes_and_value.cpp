#include "tool_place_refdes_and_value.hpp"
#include "imp/imp_interface.hpp"
#include <iostream>
#include "nlohmann/json.hpp"
#include "common/text.hpp"
#include "document/idocument.hpp"

namespace horizon {

bool ToolPlaceRefdesAndValue::can_begin()
{
    return doc.y;
}

void ToolPlaceRefdesAndValue::update_texts(const Coordi &c)
{
    text_refdes->placement.shift = {c.x, std::abs(c.y)};
    text_value->placement.shift = {c.x, -std::abs(c.y)};
}

ToolResponse ToolPlaceRefdesAndValue::begin(const ToolArgs &args)
{

    text_refdes = doc.r->insert_text(UUID::random());
    text_refdes->layer = 0;
    text_refdes->text = "$REFDES";

    text_value = doc.r->insert_text(UUID::random());
    text_value->layer = 0;
    text_value->text = "$VALUE";
    imp->tool_bar_set_actions({
            {InToolActionID::LMB},
            {InToolActionID::RMB},
    });

    update_texts(args.coords);

    return ToolResponse();
}
ToolResponse ToolPlaceRefdesAndValue::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        update_texts(args.coords);
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB:
            selection.clear();
            selection.emplace(text_value->uuid, ObjectType::TEXT);
            selection.emplace(text_refdes->uuid, ObjectType::TEXT);
            return ToolResponse::commit();

        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            return ToolResponse::revert();

        default:;
        }
    }
    return ToolResponse();
}
} // namespace horizon
