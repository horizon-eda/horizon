#include "tool_edit_pad_parameter_set.hpp"
#include "document/idocument_package.hpp"
#include "pool/package.hpp"
#include "imp/imp_interface.hpp"
#include "dialogs/pad_parameter_set_window.hpp"
#include <iostream>

namespace horizon {

bool ToolEditPadParameterSet::can_begin()
{
    return get_pads().size() > 0;
}

std::set<Pad *> ToolEditPadParameterSet::get_pads()
{
    std::set<Pad *> my_pads;
    for (const auto &it : selection) {
        if (it.type == ObjectType::PAD) {
            my_pads.emplace(&doc.k->get_package().pads.at(it.uuid));
        }
    }
    return my_pads;
}

ToolResponse ToolEditPadParameterSet::begin(const ToolArgs &args)
{
    pads = get_pads();
    selection.clear();
    auto &hl = imp->get_highlights();
    hl.clear();
    for (const auto &it : pads) {
        hl.emplace(ObjectType::PAD, it->uuid);
    }
    imp->update_highlights();

    imp->tool_bar_set_actions({
            {InToolActionID::LMB, "pick pad"},
    });

    win = imp->dialogs.show_pad_parameter_set_window(pads, doc.r->get_pool(), doc.k->get_package());
    return ToolResponse();
}

ToolResponse ToolEditPadParameterSet::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::DATA) {
        if (auto data = dynamic_cast<const ToolDataWindow *>(args.data.get())) {
            if (data->event == ToolDataWindow::Event::CLOSE) {
                select_pads();
                return ToolResponse::revert();
            }
            else if (data->event == ToolDataWindow::Event::OK) {
                select_pads();
                return ToolResponse::commit();
            }
        }
    }
    else if (args.type == ToolEventType::ACTION && args.action == InToolActionID::LMB) {
        if (args.target.type == ObjectType::PAD) {
            if (!win->go_to_pad(args.target.path.at(0))) {
                imp->tool_bar_flash("pad not selected");
            }
        }
        else {
            imp->tool_bar_flash("please click on a pad");
        }
    }
    return ToolResponse();
}

void ToolEditPadParameterSet::select_pads()
{
    for (auto &it : pads) {
        selection.emplace(it->uuid, ObjectType::PAD);
    }
}

} // namespace horizon
