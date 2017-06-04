#include "tool_add_component.hpp"
#include <iostream>
#include "core_schematic.hpp"
#include "imp_interface.hpp"
#include "entity.hpp"

namespace horizon {

	ToolAddComponent::ToolAddComponent(Core *c, ToolID tid): ToolBase(c, tid) {
		name = "Add Component";
	}

	bool ToolAddComponent::can_begin() {
        return core.c;
	}

	ToolResponse ToolAddComponent::begin(const ToolArgs &args) {
		std::cout << "tool add comp\n";
		bool r;
		UUID entity_uuid;
		std::tie(r, entity_uuid) = imp->dialogs.select_entity(core.r->m_pool);
		if(!r) {
			return ToolResponse::end();
		}
		Schematic *sch = core.c->get_schematic();
		auto uu = UUID::random();
		Component &comp = sch->block->components.emplace(uu, uu).first->second;
		comp.entity = core.c->m_pool->get_entity(entity_uuid);
		comp.refdes = comp.entity->prefix + "?";

		core.c->selection.clear();
		core.c->selection.emplace(uu, ObjectType::COMPONENT);
		core.c->commit();
		return ToolResponse::next(ToolID::MAP_SYMBOL);
	}
	ToolResponse ToolAddComponent::update(const ToolArgs &args) {

		return ToolResponse();
	}

}
