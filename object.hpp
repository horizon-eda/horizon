#pragma once
#include "uuid.hpp"

namespace horizon {
	
	class Object {
		public :
			virtual class Line *get_line(const UUID &uu) {
				return nullptr;
			}
			virtual class Junction *get_junction(const UUID &uu) {
				return nullptr;
			}
			virtual class Unit *get_unit(const UUID &uu) {
				return nullptr;
			}
			virtual class SymbolPin *get_symbol_pin(const UUID &uu) {
				return nullptr;
			}
			virtual class Entity *get_entity(const UUID &uu) {
				return nullptr;
			}
			virtual class Net *get_net(const UUID &uu) {
				return nullptr;
			}
			virtual class Symbol *get_symbol(const UUID &uu) {
				return nullptr;
			}
	};
	
	
}
