#include "selection_filter.hpp"
#include "board/board_layers.hpp"
#include "canvas_gl.hpp"

namespace horizon {
bool SelectionFilter::can_select(const SelectableRef &sel) const
{
    if (!ca.layer_is_visible(sel.layer) && sel.layer.start() != 10000 && sel.type != ObjectType::BOARD_PACKAGE)
        return false;
    if (sel.type == ObjectType::PICTURE && !ca.show_pictures)
        return false;
    ObjectType type = sel.type;
    if (type == ObjectType::POLYGON_ARC_CENTER || sel.type == ObjectType::POLYGON_EDGE
        || sel.type == ObjectType::POLYGON_VERTEX)
        type = ObjectType::POLYGON;

    if (object_filter.count(type)) {
        const auto &filter = object_filter.at(type);
        for (const auto [layer, enabled] : filter.layers) {
            if (sel.layer.overlaps(layer) && enabled)
                return true;
        }
        return filter.other_layers;
    }
    else {
        return true;
    }
    return true;
}
} // namespace horizon
