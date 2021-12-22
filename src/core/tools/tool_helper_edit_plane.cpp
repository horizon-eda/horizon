#include "tool_helper_edit_plane.hpp"
#include "dialogs/edit_plane_window.hpp"
#include "imp/imp_interface.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"

namespace horizon {
void ToolHelperEditPlane::show_edit_plane_window(class Plane &plane, class Board &brd)
{
    win = imp->dialogs.show_edit_plane_window(plane, brd);
}

ToolResponse ToolHelperEditPlane::update_for_plane(const ToolArgs &args)
{
    if (args.type == ToolEventType::DATA) {
        if (auto data = dynamic_cast<const ToolDataEditPlane *>(args.data.get())) {
            pick_net_mode = data->pick_net;
            if (pick_net_mode)
                imp->tool_bar_set_actions({
                        {InToolActionID::LMB, "pick net"},
                        {InToolActionID::RMB, "cancel"},
                });
            else
                imp->tool_bar_set_actions({});
        }
        else if (auto dataw = dynamic_cast<const ToolDataWindow *>(args.data.get())) {
            if (dataw->event == ToolDataWindow::Event::CLOSE)
                return ToolResponse::revert();
            else if (dataw->event == ToolDataWindow::Event::OK)
                return ToolResponse::commit();
        }
    }
    else if (pick_net_mode && args.type == ToolEventType::ACTION) {
        if (args.action == InToolActionID::LMB) {
            UUID net;
            const auto &brd = *doc.b->get_board();
            if (args.target.type == ObjectType::JUNCTION) {
                auto &ju = brd.junctions.at(args.target.path.at(0));
                if (ju.net)
                    net = ju.net->uuid;
            }
            else if (args.target.type == ObjectType::PAD) {
                auto &pkg = brd.packages.at(args.target.path.at(0));
                auto &pad = pkg.package.pads.at(args.target.path.at(1));
                if (pad.net) {
                    net = pad.net->uuid;
                }
            }
            if (net)
                win->set_net(net);
        }
        else if (args.action == InToolActionID::RMB) {
            win->set_net(UUID());
        }
    }
    return ToolResponse();
}

} // namespace horizon
