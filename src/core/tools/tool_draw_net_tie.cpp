#include "tool_draw_net_tie.hpp"
#include "board/board_rules.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include "imp/imp_interface.hpp"

namespace horizon {

bool ToolDrawNetTie::can_begin()
{
    if (!doc.b)
        return false;
    return doc.b->get_top_block()->net_ties.size();
}

ToolResponse ToolDrawNetTie::begin(const ToolArgs &args)
{
    selection.clear();
    std::set<NetTie *> ties;
    for (auto &[uu, tie] : doc.b->get_top_block()->net_ties) {
        ties.insert(&tie);
    }
    for (auto &[uu, tie] : doc.b->get_board()->net_ties) {
        ties.erase(tie.net_tie);
    }
    if (ties.size() == 0) {
        imp->tool_bar_flash("not ne ties left to map");
        return ToolResponse::end();
    }
    auto uu = imp->dialogs.map_net_tie(ties);
    if (!uu)
        return ToolResponse::end();
    net_tie = &doc.b->get_top_block()->net_ties.at(*uu);

    imp->tool_bar_set_tip("drawing net tie for nets " + net_tie->net_primary->name + " and "
                          + net_tie->net_secondary->name);

    imp->tool_bar_set_actions({
            {InToolActionID::LMB},
            {InToolActionID::RMB, "finish"},
    });

    rules = dynamic_cast<BoardRules *>(doc.r->get_rules());
    return ToolResponse();
}

void ToolDrawNetTie::create_temp_tie()
{
    auto uu = UUID::random();
    temp_tie = &doc.b->get_board()->net_ties.emplace(uu, uu).first->second;
    temp_tie->net_tie = net_tie;
    temp_tie->width_from_rules = true;
    temp_junc = &doc.b->get_board()->junctions.emplace(uu, uu).first->second;
    temp_tie->to = temp_junc;
    imp->set_snap_filter({{ObjectType::JUNCTION, temp_junc->uuid}});
}

ToolResponse ToolDrawNetTie::update(const ToolArgs &args)
{
    auto &brd = *doc.b->get_board();
    if (args.type == ToolEventType::MOVE) {
        if (temp_junc) {
            temp_junc->position = args.coords;
            if (temp_junc->net) {
                std::set<UUID> nets;
                nets.insert(temp_junc->net->uuid);
                brd.update_airwires(true, nets);
            }
        }
        return ToolResponse();
    }
    else if (args.type == ToolEventType::ACTION) {
        switch (args.action) {
        case InToolActionID::LMB:
            if (temp_junc) {
                if (args.target.type == ObjectType::JUNCTION) {
                    auto &ju = brd.junctions.at(args.target.path.at(0));

                    Net *required_net = nullptr;
                    if (temp_tie->from->net == net_tie->net_primary)
                        required_net = net_tie->net_secondary;
                    else if (temp_tie->from->net == net_tie->net_secondary)
                        required_net = net_tie->net_primary;

                    if (ju.net == required_net) {
                        temp_tie->to = &ju;
                        brd.junctions.erase(temp_junc->uuid);
                        temp_junc = nullptr;
                        return ToolResponse::commit();
                    }
                    else {
                        imp->tool_bar_flash("can't connect to this net");
                    }
                }
                else if (args.target.type == ObjectType::INVALID) {
                    return ToolResponse::commit();
                }
                else {
                    imp->tool_bar_flash("unsupported target");
                }
            }
            else {
                // start
                if (args.target.type == ObjectType::JUNCTION) {
                    auto &ju = brd.junctions.at(args.target.path.at(0));
                    if (ju.net == net_tie->net_primary || ju.net == net_tie->net_secondary) {
                        create_temp_tie();
                        temp_tie->from = &ju;
                        temp_tie->layer = ju.layer.start();
                        temp_junc->layer = temp_tie->layer;
                        temp_junc->position = args.coords;
                        temp_tie->width = rules->get_default_track_width(net_tie->net_primary, temp_tie->layer);
                    }
                    else {
                        imp->tool_bar_flash("only connects to nets of this net tie");
                    }
                }
                else if (args.target.type == ObjectType::INVALID) {
                    create_temp_tie();
                    auto &ju = dynamic_cast<BoardJunction &>(*doc.b->insert_junction(UUID::random()));
                    ju.position = args.coords;
                    temp_tie->from = &ju;
                    temp_tie->layer = args.work_layer;
                    temp_junc->layer = temp_tie->layer;
                    temp_junc->position = args.coords;
                    temp_tie->width = rules->get_default_track_width(net_tie->net_primary, temp_tie->layer);
                }
                else {
                    imp->tool_bar_flash("unsupported target");
                }
            }
            break;

        case InToolActionID::RMB:
        case InToolActionID::CANCEL:
            return ToolResponse::revert();

        default:;
        }
    }
    return ToolResponse();
}
} // namespace horizon
