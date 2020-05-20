#include "selection_filter.hpp"
#include "board/board_layers.hpp"
#include "canvas.hpp"

namespace horizon {
bool SelectionFilter::can_select(const SelectableRef &sel) const
{
    if (!ca->layer_is_visible(sel.layer) && sel.layer != 10000 && sel.type != ObjectType::BOARD_PACKAGE)
        return false;
    ObjectType type = sel.type;
    if (type == ObjectType::POLYGON_ARC_CENTER || sel.type == ObjectType::POLYGON_EDGE
        || sel.type == ObjectType::POLYGON_VERTEX)
        type = ObjectType::POLYGON;

    if (object_filter.count(type)) {
        const auto &filter = object_filter.at(type);
        if (filter.layers.count(sel.layer)) {
            return filter.layers.at(sel.layer);
        }
        else {
            return filter.other_layers;
        }
    }
    else {
        return true;
    }
    return true;
}
} // namespace horizon
