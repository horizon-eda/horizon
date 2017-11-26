#include "pool_notebook.hpp"
#include "editor_window.hpp"
#include "duplicate/duplicate_unit.hpp"
#include "util.hpp"

namespace horizon {
	void PoolNotebook::handle_edit_entity(const UUID &uu) {
		if(!uu)
			return;
		auto path = pool.get_filename(ObjectType::ENTITY, uu);
		spawn(PoolManagerProcess::Type::ENTITY, {path});
	}

	void PoolNotebook::handle_create_entity() {
		spawn(PoolManagerProcess::Type::ENTITY, {""});
	}

	void PoolNotebook::handle_duplicate_entity(const UUID &uu) {
		if(!uu)
			return;
		show_duplicate_window(ObjectType::ENTITY, uu);
	}
}

