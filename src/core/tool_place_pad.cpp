#include "tool_place_pad.hpp"
#include "document/idocument_package.hpp"
#include "pool/package.hpp"
#include "imp/imp_interface.hpp"
#include <iostream>
#include "pool/pool.hpp"
#include <gdk/gdkkeysyms.h>

namespace horizon {

ToolPlacePad::ToolPlacePad(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

bool ToolPlacePad::can_begin()
{
    return doc.k;
}

ToolResponse ToolPlacePad::begin(const ToolArgs &args)
{
    std::cout << "tool add comp\n";
    bool r;
    UUID padstack_uuid;
    std::tie(r, padstack_uuid) = imp->dialogs.select_padstack(doc.r->get_pool(), doc.k->get_package()->uuid);
    if (!r) {
        return ToolResponse::end();
    }

    padstack = doc.r->get_pool()->get_padstack(padstack_uuid);
    create_pad(args.coords);

    imp->tool_bar_set_tip(
            "<b>LMB:</b>place pad <b>RMB:</b>delete current pad and finish <b>i:</b>edit pad <b>r:</b>rotate");
    return ToolResponse();
}

void ToolPlacePad::create_pad(const Coordi &pos)
{
    Package *pkg = doc.k->get_package();
    auto uu = UUID::random();
    temp = &pkg->pads.emplace(uu, Pad(uu, padstack)).first->second;
    temp->placement.shift = pos;
    for (auto &p : padstack->parameters_required) {
        temp->parameter_set[p] = padstack->parameter_set.at(p);
    }
    if (pkg->pads.size() == 1) { // first pad
        temp->name = "1";
    }
    else {
        int max_name = pkg->get_max_pad_name();
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
    else if (args.type == ToolEventType::CLICK) {
        if (args.button == 1) {
            auto old_pad = temp;
            create_pad(args.coords);
            temp->placement.set_angle(old_pad->placement.get_angle());
            temp->parameter_set = old_pad->parameter_set;
            doc.k->get_package()->apply_parameter_set({});
        }
        else if (args.button == 3) {
            doc.k->get_package()->pads.erase(temp->uuid);
            temp = 0;
            selection.clear();
            return ToolResponse::commit();
        }
    }
    else if (args.type == ToolEventType::KEY) {
        if (args.key == GDK_KEY_Escape) {
            return ToolResponse::revert();
        }
        else if (args.key == GDK_KEY_r) {
            temp->placement.inc_angle_deg(90);
        }
        else if (args.key == GDK_KEY_i) {
            std::set<Pad *> pads{temp};
            auto params = temp->parameter_set;
            if (imp->dialogs.edit_pad_parameter_set(pads, doc.r->get_pool(), doc.k->get_package())
                == false) { // rollback
                temp->parameter_set = params;
            }
            doc.k->get_package()->apply_parameter_set({});
        }
    }
    return ToolResponse();
}
} // namespace horizon
