#include "tool_draw_connection_line.hpp"
#include "imp/imp_interface.hpp"
#include "pool/part.hpp"
#include <algorithm>
#include <nlohmann/json.hpp>
#include "document/idocument_board.hpp"
#include "board/board.hpp"

namespace horizon {

bool ToolDrawConnectionLine::can_begin()
{
    return doc.b;
}

ToolResponse ToolDrawConnectionLine::begin(const ToolArgs &args)
{
    const auto uu = UUID::random();
    temp_junc = &doc.b->get_board()->junctions.emplace(uu, uu).first->second;
    imp->set_snap_filter({{ObjectType::JUNCTION, temp_junc->uuid}});
    temp_junc->position = args.coords;
    temp_line = nullptr;
    update_tip();
    return ToolResponse();
}

void ToolDrawConnectionLine::update_tip()
{
    std::vector<ActionLabelInfo> actions;
    actions.reserve(2);
    actions.emplace_back(InToolActionID::LMB, "connect");
    if (temp_line)
        actions.emplace_back(InToolActionID::RMB, "cancel current line");
    else
        actions.emplace_back(InToolActionID::RMB, "end tool");
    imp->tool_bar_set_actions(actions);
}

ToolResponse ToolDrawConnectionLine::update(const ToolArgs &args)
{
    auto brd = doc.b->get_board();
    if (args.type == ToolEventType::MOVE) {
        if (temp_line)
            temp_junc->position = args.coords;
        return ToolResponse();
    }
    else if (args.type == ToolEventType::ACTION) {
        const ObjectType target_type = args.target.type;
        switch (args.action) {
        case InToolActionID::LMB:
            if (target_type == ObjectType::PAD || target_type == ObjectType::BOARD_PACKAGE) {

                auto pkgpad = pad_from_target(args.target);

                if (!pkgpad) {
                    imp->tool_bar_flash("only connects to pads or junctions");
                    break;
                }
                auto pkg = &pkgpad->pkg;
                auto pad = &pkgpad->pad;

                if (temp_line) {
                    if (temp_net == nullptr || pad->net == nullptr) {
                        temp_line->to.connect(pkg, pad);
                        temp_line_last2 = temp_line_last;
                        temp_line_last = temp_line;
                        temp_line = nullptr;
                        temp_net = nullptr;
                    }
                    else {
                        imp->tool_bar_flash("can't connect two nets");
                    }
                }
                else {
                    auto uu = UUID::random();
                    temp_line = &brd->connection_lines.emplace(uu, uu).first->second;
                    temp_line->from.connect(pkg, pad);
                    temp_line->to.connect(temp_junc);
                    temp_junc->position = args.coords;
                    temp_net = pad->net;
                }
            }
            else if (target_type == ObjectType::JUNCTION) {
                auto &ju = brd->junctions.at(args.target.path.at(0));
                if (ju.net) {
                    if (temp_line) {
                        if (temp_net == nullptr || ju.net == nullptr) {
                            temp_line->to.connect(&ju);
                            temp_line_last2 = temp_line_last;
                            temp_line_last = temp_line;
                            temp_line = nullptr;
                            temp_net = nullptr;
                        }
                        else {
                            imp->tool_bar_flash("can't connect two nets");
                        }
                    }
                    else {
                        auto uu = UUID::random();
                        temp_line = &brd->connection_lines.emplace(uu, uu).first->second;
                        temp_line->from.connect(&ju);
                        temp_line->to.connect(temp_junc);
                        temp_junc->position = args.coords;
                        temp_net = ju.net;
                    }
                }
                else {
                    imp->tool_bar_flash("junction has no net");
                }
            }
            else {
                imp->tool_bar_flash("only connects to pads or junctions");
            }
            break;

        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            if (temp_line) {
                doc.b->get_board()->connection_lines.erase(temp_line->uuid);
                temp_line = nullptr;
            }
            else {
                doc.r->delete_junction(temp_junc->uuid);
                temp_junc = nullptr;
                return ToolResponse::commit();
            }
            break;

        default:;
        }
    }
    update_tip();
    return ToolResponse();
}
} // namespace horizon
