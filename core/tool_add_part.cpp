#include "tool_add_part.hpp"
#include <iostream>
#include "core_schematic.hpp"
#include "part.hpp"

namespace horizon {

	ToolAddPart::ToolAddPart(Core *c, ToolID tid): ToolBase(c, tid) {
		name = "Add Part";
	}

	bool ToolAddPart::can_begin() {
        return core.c;
	}

	ToolResponse ToolAddPart::begin(const ToolArgs &args) {
		std::cout << "tool add part\n";
		bool r;
		UUID part_uuid;
		std::tie(r, part_uuid) = core.r->dialogs.select_part(UUID(), UUID());
		if(!r) {
			return ToolResponse::end();
		}
		auto part = core.c->m_pool->get_part(part_uuid);
		Schematic *sch = core.c->get_schematic();
		auto uu = UUID::random();
		Component &comp = sch->block->components.emplace(uu, uu).first->second;
		comp.entity = part->entity;
		comp.part = part;
		comp.refdes = comp.entity->prefix + "?";

		core.c->selection.clear();
		core.c->selection.emplace(uu, ObjectType::COMPONENT);
		core.c->commit();
		return ToolResponse::next(ToolID::MAP_SYMBOL);
	}
	ToolResponse ToolAddPart::update(const ToolArgs &args) {

		return ToolResponse();
	}

}
