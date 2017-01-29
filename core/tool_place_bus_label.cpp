#include "tool_place_bus_label.hpp"
#include <iostream>
#include "core_schematic.hpp"

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
		std::tie(r, net_uuid) = core.r->dialogs.select_bus();
		if(!r) {
			return false;
		}
		bus = &core.c->get_schematic()->block->buses.at(net_uuid);
		return true;
	}

	void ToolPlaceBusLabel::create_attached() {
		auto uu = UUID::random();
		la = &core.c->get_sheet()->bus_labels.emplace(uu, uu).first->second;
		la->bus =bus;
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
		/*else if(args.type == ToolEventType::KEY) {
			if(la) {
				if(args.key == GDK_KEY_r) {
					la->orientation = transform_orienation(la->orientation, true);
					last_orientation = la->orientation;
					return true;
				}
			}
		}*/

		return false;
	}

}
