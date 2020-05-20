#include "tool_draw_connection_line.hpp"
#include "imp/imp_interface.hpp"
#include "pool/part.hpp"
#include <algorithm>
#include "nlohmann/json.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"

namespace horizon {

ToolDrawConnectionLine::ToolDrawConnectionLine(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

bool ToolDrawConnectionLine::can_begin()
{
    return doc.b;
}

ToolResponse ToolDrawConnectionLine::begin(const ToolArgs &args)
{
    temp_junc = doc.r->insert_junction(UUID::random());
    imp->set_snap_filter({{ObjectType::JUNCTION, temp_junc->uuid}});
    temp_junc->position = args.coords;
    temp_line = nullptr;
    update_tip();
    return ToolResponse();
}

void ToolDrawConnectionLine::update_tip()
{
    std::string s("<b>LMB:</b>connect <b>RMB:</b>");
    if (temp_line) {
        s += "cancel current line";
    }
    else {
        s += "end tool";
    }
    imp->tool_bar_set_tip(s);
}

ToolResponse ToolDrawConnectionLine::update(const ToolArgs &args)
{
    auto brd = doc.b->get_board();
    if (args.type == ToolEventType::MOVE) {
        if (temp_line)
            temp_junc->position = args.coords;
        return ToolResponse::fast();
    }
    else if (args.type == ToolEventType::CLICK) {
        if (args.button == 1) {
            if (args.target.type == ObjectType::PAD) {
                auto pkg = &brd->packages.at(args.target.path.at(0));
                auto pad = &pkg->package.pads.at(args.target.path.at(1));
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
            else if (args.target.type == ObjectType::JUNCTION) {
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
        }
        else if (args.button == 3) {
            if (temp_line) {
                doc.b->get_board()->connection_lines.erase(temp_line->uuid);
                temp_line = nullptr;
            }
            else {
                doc.r->delete_junction(temp_junc->uuid);
                temp_junc = nullptr;
                return ToolResponse::commit();
            }
        }
    }
    update_tip();
    return ToolResponse();
}
} // namespace horizon
