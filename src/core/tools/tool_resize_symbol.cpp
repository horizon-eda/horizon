#include "tool_resize_symbol.hpp"
#include "document/idocument_symbol.hpp"
#include "pool/symbol.hpp"
#include "util/util.hpp"
#include "imp/imp_interface.hpp"

namespace horizon {

ToolResizeSymbol::ToolResizeSymbol(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

bool ToolResizeSymbol::can_begin()
{
    return doc.y;
}

ToolResponse ToolResizeSymbol::begin(const ToolArgs &args)
{

    pos_orig = args.coords;
    const auto &sym = *doc.y->get_symbol();
    for (const auto &it : sym.pins) {
        positions.emplace(std::piecewise_construct, std::forward_as_tuple(ObjectType::SYMBOL_PIN, it.first),
                          std::forward_as_tuple(it.second.position));
    }
    for (const auto &it : sym.junctions) {
        positions.emplace(std::piecewise_construct, std::forward_as_tuple(ObjectType::JUNCTION, it.first),
                          std::forward_as_tuple(it.second.position));
    }
    for (const auto &it : sym.texts) {
        positions.emplace(std::piecewise_construct, std::forward_as_tuple(ObjectType::TEXT, it.first),
                          std::forward_as_tuple(it.second.placement.shift));
    }
    update_positions(args.coords);
    return ToolResponse();
}

void ToolResizeSymbol::update_positions(const Coordi &ac)
{
    auto d = ac - pos_orig;
    if (pos_orig.y < 0)
        d.y *= -1;
    if (pos_orig.x < 0)
        d.x *= -1;
    auto c = d + delta_key;
    imp->tool_bar_set_tip("<b>LMB:</b>finish <b>RMB:</b>cancel <i>" + coord_to_string(c, true) + "</i>");
    auto &sym = *doc.y->get_symbol();
    for (auto &it : sym.pins) {
        const auto k = std::make_pair(ObjectType::SYMBOL_PIN, it.first);
        const auto &p = positions.at(k);
        switch (it.second.orientation) {
        case Orientation::RIGHT:
            it.second.position = p + Coordi(c.x, 0);
            break;
        case Orientation::LEFT:
            it.second.position = p + Coordi(-c.x, 0);
            break;
        case Orientation::UP:
            it.second.position = p + Coordi(0, c.y);
            break;
        case Orientation::DOWN:
            it.second.position = p + Coordi(0, -c.y);
            break;
        }
    }
    for (auto &it : sym.junctions) {
        const auto k = std::make_pair(ObjectType::JUNCTION, it.first);
        const auto &p = positions.at(k);
        if (p.x > 0 && p.y > 0) {
            it.second.position = p + Coordi(c.x, c.y);
        }
        else if (p.x > 0 && p.y < 0) {
            it.second.position = p + Coordi(c.x, -c.y);
        }
        else if (p.x < 0 && p.y < 0) {
            it.second.position = p + Coordi(-c.x, -c.y);
        }
        else if (p.x < 0 && p.y > 0) {
            it.second.position = p + Coordi(-c.x, c.y);
        }
    }
    for (auto &it : sym.texts) {
        const auto k = std::make_pair(ObjectType::TEXT, it.first);
        const auto &p = positions.at(k);
        if (p.y > 0) {
            it.second.placement.shift = p + Coordi(0, c.y);
        }
        else {
            it.second.placement.shift = p + Coordi(0, -c.y);
        }
    }
}

ToolResponse ToolResizeSymbol::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        update_positions(args.coords);
    }
    else if (args.type == ToolEventType::KEY) {
        auto dir = dir_from_arrow_key(args.key);
        if (dir.x || dir.y) {
            delta_key += dir * 1.25_mm;
            update_positions(args.coords);
        }
    }
    else if (args.type == ToolEventType::CLICK) {
        if (args.button == 1) {
            return ToolResponse::commit();
        }
        else if (args.button == 3) {
            return ToolResponse::revert();
        }
    }
    return ToolResponse();
}
} // namespace horizon
