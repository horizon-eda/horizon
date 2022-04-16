#include "tool_helper_save_placements.hpp"
#include "document/idocument_board.hpp"
#include "document/idocument_package.hpp"
#include "board/board.hpp"
#include "pool/package.hpp"
#include "util/text_renderer.hpp"

namespace horizon {

ToolHelperSavePlacements::PlacementInfo::PlacementInfo(const Placement &p, const std::pair<Coordi, Coordi> &bbox)
    : placement(p), top(bbox.second.y - p.shift.y), left(p.shift.x - bbox.first.x), right(bbox.second.x - p.shift.x),
      bottom(p.shift.y - bbox.first.y)
{
}

bool ToolHelperSavePlacements::type_is_supported(ObjectType type)
{
    switch (type) {
    case ObjectType::JUNCTION:
    case ObjectType::POLYGON_VERTEX:
    case ObjectType::POLYGON_ARC_CENTER:
    case ObjectType::TEXT:
    case ObjectType::BOARD_PACKAGE:
    case ObjectType::BOARD_DECAL:
    case ObjectType::PICTURE:
    case ObjectType::PAD:
        return true;

    default:
        return false;
    }
}

size_t ToolHelperSavePlacements::count_types_supported() const
{
    return std::count_if(selection.begin(), selection.end(), [](auto &x) { return type_is_supported(x.type); });
}

void ToolHelperSavePlacements::save_placements()
{
    placements.clear();
    for (const auto &it : selection) {
        switch (it.type) {
        case ObjectType::JUNCTION:
            placements.emplace(std::piecewise_construct, std::forward_as_tuple(it),
                               std::forward_as_tuple(doc.r->get_junction(it.uuid)->position));
            break;
        case ObjectType::POLYGON_VERTEX:
            placements.emplace(std::piecewise_construct, std::forward_as_tuple(it),
                               std::forward_as_tuple(doc.r->get_polygon(it.uuid)->vertices.at(it.vertex).position));
            break;
        case ObjectType::POLYGON_ARC_CENTER:
            placements.emplace(std::piecewise_construct, std::forward_as_tuple(it),
                               std::forward_as_tuple(doc.r->get_polygon(it.uuid)->vertices.at(it.vertex).arc_center));
            break;
        case ObjectType::TEXT: {
            const auto text = doc.r->get_text(it.uuid);
            TextRenderer tr;
            const bool rev = doc.r->get_layer_provider().get_layers().at(text->layer).reverse;
            const auto extents = tr.render(*text, ColorP::FROM_LAYER, Placement(), rev);
            const auto bb = std::make_pair(extents.first.to_coordi(), extents.second.to_coordi());
            placements.emplace(std::piecewise_construct, std::forward_as_tuple(it),
                               std::forward_as_tuple(text->placement, bb));
        } break;
        case ObjectType::BOARD_PACKAGE: {
            const auto &pkg = doc.b->get_board()->packages.at(it.uuid);
            const auto bb = pkg.get_bbox();
            placements.emplace(std::piecewise_construct, std::forward_as_tuple(it),
                               std::forward_as_tuple(pkg.placement, bb));
        } break;
        case ObjectType::BOARD_DECAL: {
            const auto &dec = doc.b->get_board()->decals.at(it.uuid);
            placements.emplace(std::piecewise_construct, std::forward_as_tuple(it),
                               std::forward_as_tuple(dec.placement));
            decal_scales.emplace(std::piecewise_construct, std::forward_as_tuple(it.uuid),
                                 std::forward_as_tuple(dec.get_scale()));
        } break;
        case ObjectType::PICTURE: {
            const auto pic = doc.r->get_picture(it.uuid);
            placements.emplace(std::piecewise_construct, std::forward_as_tuple(it),
                               std::forward_as_tuple(pic->placement));
            picture_px_sizes.emplace(std::piecewise_construct, std::forward_as_tuple(it.uuid),
                                     std::forward_as_tuple(pic->px_size));

        } break;
        case ObjectType::PAD: {
            const auto &pad = doc.k->get_package().pads.at(it.uuid);
            placements.emplace(
                    std::piecewise_construct, std::forward_as_tuple(it),
                    std::forward_as_tuple(pad.placement, pad.placement.transform_bb(pad.padstack.get_bbox())));
        } break;
        default:;
        }
    }
}

void ToolHelperSavePlacements::apply_placements(Callback fn)
{
    for (const auto &[sel, it] : placements) {
        switch (sel.type) {
        case ObjectType::JUNCTION:
            doc.r->get_junction(sel.uuid)->position = fn(sel, it).shift;
            break;
        case ObjectType::POLYGON_VERTEX:
            doc.r->get_polygon(sel.uuid)->vertices.at(sel.vertex).position = fn(sel, it).shift;
            break;
        case ObjectType::POLYGON_ARC_CENTER:
            doc.r->get_polygon(sel.uuid)->vertices.at(sel.vertex).arc_center = fn(sel, it).shift;
            break;
        case ObjectType::TEXT:
            doc.r->get_text(sel.uuid)->placement = fn(sel, it);
            break;
        case ObjectType::BOARD_PACKAGE:
            doc.b->get_board()->packages.at(sel.uuid).placement = fn(sel, it);
            break;
        case ObjectType::BOARD_DECAL:
            doc.b->get_board()->decals.at(sel.uuid).placement = fn(sel, it);
            break;
        case ObjectType::PICTURE:
            doc.r->get_picture(sel.uuid)->placement = fn(sel, it);
            break;
        case ObjectType::PAD:
            doc.k->get_package().pads.at(sel.uuid).placement = fn(sel, it);
            break;
        default:;
        }
    }
}

void ToolHelperSavePlacements::reset_placements()
{
    apply_placements([](const SelectableRef &sel, const PlacementInfo &it) { return it.placement; });
}

void ToolHelperSavePlacements::update_indices(IndexCallback fn)
{
    std::list<std::pair<const SelectableRef &, const PlacementInfo &>> items;
    for (const auto &[sel, it] : placements) {
        items.emplace_back(sel, it);
    }
    items.sort([&fn](const auto &a, const auto &b) { return fn(a.first, a.second) < fn(b.first, b.second); });
    size_t i = 0;
    for (const auto &[sel, it] : items) {
        placements.at(sel).index = i++;
    }
}

const ToolHelperSavePlacements::PlacementPair &
ToolHelperSavePlacements::get_placement_info_for_index(size_t index) const
{
    for (const auto &it : placements) {
        if (it.second.index == index)
            return it;
    }
    throw std::runtime_error("index not found");
}

} // namespace horizon
