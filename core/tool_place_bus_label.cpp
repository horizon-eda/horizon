#include "tool_place_bus_label.hpp"
#include "tool_helper_move.hpp"
#include <iostream>
#include "core_schematic.hpp"
#include "imp_interface.hpp"

namespace horizon {

	ToolPlaceBusLabel::ToolPlaceBusLabel(Core *c, ToolID tid):ToolPlaceJunction(c, tid) {
		name = "Place bus label";
	}

	bool ToolPlaceBusLabel::can_begin() {
		return core.c;
	}

	bool ToolPlaceBusLabel::begin_attached() {
		bool r;
		UUID net_uuid;
		std::tie(r, net_uuid) = imp->dialogs.select_bus(core.c->get_schematic()->block);
		if(!r) {
			return false;
		}
		bus = &core.c->get_schematic()->block->buses.at(net_uuid);
		imp->tool_bar_set_tip("<b>LMB:</b>place bus label <b>RMB:</b>delete current label and finish <b>r:</b>rotate <b>e:</b>mirror");
		return true;
	}

	void ToolPlaceBusLabel::create_attached() {
		auto uu = UUID::random();
		la = &core.c->get_sheet()->bus_labels.emplace(uu, uu).first->second;
		la->bus =bus;
		la->orientation = last_orientation;
		la->junction = temp;
		temp->bus = bus;
	}

	void ToolPlaceBusLabel::delete_attached() {
		if(la) {
			core.c->get_sheet()->bus_labels.erase(la->uuid);
			temp->net = nullptr;
		}
	}

	bool ToolPlaceBusLabel::check_line(LineNet *li) {
		if(li->net)
			return false;
		if(!li->bus)
			return true;
		if(li->bus != bus)
			return false;
		return true;
	}

	bool ToolPlaceBusLabel::update_attached(const ToolArgs &args) {
		if(args.type == ToolEventType::CLICK) {
			if(args.button == 1) {
				if(args.target.type == ObjectType::JUNCTION) {
					Junction *j = core.r->get_junction(args.target.path.at(0));
					if(j->net)
						return true;
					if(j->bus && j->bus != bus)
						return true;
					la->junction = j;
					j->bus = bus;
					create_attached();
					return true;
				}
			}
		}
		if(args.key == GDK_KEY_r || args.key == GDK_KEY_e) {
			la->orientation = ToolHelperMove::transform_orienation(la->orientation, args.key == GDK_KEY_r);
			last_orientation = la->orientation;
			return true;
		}


		return false;
	}

}
