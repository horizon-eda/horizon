#include "selection_filter.hpp"
#include "canvas.hpp"
#include "board/board_layers.hpp"

namespace horizon {
	bool SelectionFilter::can_select(const SelectableRef &sel) {
		if(object_filter.count(sel.type))
			if(!object_filter.at(sel.type))
				return false;
		if(sel.type == ObjectType::BOARD_PACKAGE) {
			if(work_layer_only)
				return (ca->work_layer < 0) ^ (sel.layer == BoardLayers::TOP_PACKAGE);
			else
				return true;
		}
		if(sel.layer == 10000)
			return true;
		if(work_layer_only && (ca->work_layer != sel.layer))
			return false;
		if(!ca->layer_is_visible(sel.layer))
			return false;
		return true;
	}
}
