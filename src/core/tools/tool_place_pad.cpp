#include "tool_place_pad.hpp"
#include "document/idocument_package.hpp"
#include "pool/package.hpp"
#include "imp/imp_interface.hpp"
#include <iostream>
#include "pool/pool.hpp"

namespace horizon {

bool ToolPlacePad::can_begin()
{
    return doc.k;
}

ToolResponse ToolPlacePad::begin(const ToolArgs &args)
{
    if (auto r = imp->dialogs.select_padstack(doc.r->get_pool(), doc.k->get_package().uuid)) {
        padstack = doc.r->get_pool().get_padstack(*r);
        create_pad(args.coords);

        imp->tool_bar_set_actions({
                {InToolActionID::LMB},
                {InToolActionID::RMB},
                {InToolActionID::ROTATE},
                {InToolActionID::EDIT, "edit pad"},
        });
        return ToolResponse();
    }
    else {
        return ToolResponse::end();
    }
}

void ToolPlacePad::create_pad(const Coordi &pos)
{
    auto &pkg = doc.k->get_package();
    auto uu = UUID::random();
    temp = &pkg.pads.emplace(uu, Pad(uu, padstack)).first->second;
    temp->placement.shift = pos;
    for (auto &p : padstack->parameters_required) {
        temp->parameter_set[p] = padstack->parameter_set.at(p);
    }
    if (pkg.pads.size() == 1) { // first pad
        temp->name = "1";
    }
    else {
        int max_name = pkg.get_max_pad_name();
        if (max_name > 0) {
            temp->name = std::to_string(max_name + 1);
        }
    }
}

ToolResponse ToolPlacePad::update(const ToolArgs &args)
{

    if (args.type == ToolEventType::MOVE) {
        temp->placement.shift = args.coords;
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB: {
            auto old_pad = temp;
            create_pad(args.coords);
            temp->placement.set_angle(old_pad->placement.get_angle());
            temp->parameter_set = old_pad->parameter_set;
            doc.k->get_package().apply_parameter_set({});
        } break;

        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            doc.k->get_package().pads.erase(temp->uuid);
            temp = 0;
            selection.clear();
            return ToolResponse::commit();

        case InToolActionID::ROTATE:
            temp->placement.inc_angle_deg(90);
            break;

        case InToolActionID::EDIT: {
            std::set<Pad *> pads{temp};
            auto params = temp->parameter_set;
            if (imp->dialogs.edit_pad_parameter_set(pads, doc.r->get_pool(), doc.k->get_package())
                == false) { // rollback
                temp->parameter_set = params;
            }
            doc.k->get_package().apply_parameter_set({});
        } break;

        default:;
        }
    }
    return ToolResponse();
}
} // namespace horizon
