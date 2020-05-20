#include "tool_place_net_label.hpp"
#include "document/idocument_schematic.hpp"
#include "schematic/schematic.hpp"
#include "imp/imp_interface.hpp"
#include "tool_helper_move.hpp"
#include <iostream>
#include <gdk/gdkkeysyms.h>

namespace horizon {

ToolPlaceNetLabel::ToolPlaceNetLabel(IDocument *c, ToolID tid)
    : ToolBase(c, tid), ToolPlaceJunction(c, tid), ToolHelperDrawNetSetting(c, tid)
{
}

bool ToolPlaceNetLabel::can_begin()
{
    return doc.c;
}

void ToolPlaceNetLabel::create_attached()
{
    auto uu = UUID::random();
    la = &doc.c->get_sheet()->net_labels.emplace(uu, uu).first->second;
    la->orientation = last_orientation;
    la->size = settings.net_label_size;
    la->junction = temp;
}

void ToolPlaceNetLabel::delete_attached()
{
    if (la) {
        doc.c->get_sheet()->net_labels.erase(la->uuid);
        la = nullptr;
    }
}

bool ToolPlaceNetLabel::check_line(LineNet *li)
{
    if (li->bus)
        return false;
    return true;
}

bool ToolPlaceNetLabel::begin_attached()
{
    imp->tool_bar_set_tip(
            "<b>LMB:</b>place label <b>RMB:</b>delete current label and finish "
            "<b>r:</b>rotate <b>e:</b>mirror <b>+-:</b>change label size <b>s:</b>set label size");
    return true;
}

void ToolPlaceNetLabel::apply_settings()
{
    if (la)
        la->size = settings.net_label_size;
}

bool ToolPlaceNetLabel::update_attached(const ToolArgs &args)
{
    if (args.type == ToolEventType::CLICK) {
        if (args.button == 1) {
            if (args.target.type == ObjectType::JUNCTION) {
                Junction *j = doc.r->get_junction(args.target.path.at(0));
                if (j->bus)
                    return true;
                la->junction = j;
                create_attached();
                return true;
            }
            else if (args.target.type == ObjectType::SYMBOL_PIN) {
                SchematicSymbol *schsym = doc.c->get_schematic_symbol(args.target.path.at(0));
                SymbolPin *pin = &schsym->symbol.pins.at(args.target.path.at(1));
                UUIDPath<2> connpath(schsym->gate->uuid, args.target.path.at(1));
                if (schsym->component->connections.count(connpath) == 0) {
                    // sympin not connected
                    auto uu = UUID::random();
                    auto *line = &doc.c->get_sheet()->net_lines.emplace(uu, uu).first->second;
                    line->net = doc.c->get_schematic()->block->insert_net();
                    line->from.connect(schsym, pin);
                    line->to.connect(temp);
                    schsym->component->connections.emplace(UUIDPath<2>(schsym->gate->uuid, pin->uuid),
                                                           static_cast<Net *>(line->net));

                    temp->net = line->net;
                    switch (la->orientation) {
                    case Orientation::LEFT:
                        temp->position.x -= 1.25_mm;
                        break;
                    case Orientation::RIGHT:
                        temp->position.x += 1.25_mm;
                        break;
                    case Orientation::DOWN:
                        temp->position.y -= 1.25_mm;
                        break;
                    case Orientation::UP:
                        temp->position.y += 1.25_mm;
                        break;
                    }
                    create_junction(args.coords);
                    create_attached();
                }

                return true;
            }
        }
    }
    else if (args.type == ToolEventType::KEY) {
        if (la) {
            if (args.key == GDK_KEY_r || args.key == GDK_KEY_e) {
                la->orientation = ToolHelperMove::transform_orientation(la->orientation, args.key == GDK_KEY_r);
                last_orientation = la->orientation;
                return true;
            }
            else if (args.key == GDK_KEY_plus || args.key == GDK_KEY_equal) {
                step_net_label_size(true);
                return true;
            }
            else if (args.key == GDK_KEY_minus) {
                step_net_label_size(false);
                return true;
            }
            else if (args.key == GDK_KEY_s) {
                ask_net_label_size();
                return true;
            }
        }
    }
    return false;
}
} // namespace horizon
