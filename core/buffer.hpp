#pragma once
#include <set>
#include <map>
#include "uuid.hpp"
#include "text.hpp"
#include "junction.hpp"
#include "line.hpp"
#include "arc.hpp"
#include "pad.hpp"
#include "cores.hpp"
#include "core.hpp"
#include "component.hpp"
#include "schematic_symbol.hpp"
#include "symbol.hpp"
#include "hole.hpp"
#include "polygon.hpp"
#include "net.hpp"
#include "json.hpp"

namespace horizon {
	class Buffer {
		public:
			Buffer(Core *co);
			void clear();
			void load_from_symbol(std::set<SelectableRef> selection);

			std::map<UUID, Text> texts;
			std::map<UUID, Junction> junctions;
			std::map<UUID, Line> lines;
			std::map<UUID, Arc> arcs;
			std::map<UUID, Pad> pads;
			std::map<UUID, Polygon> polygons;
			std::map<UUID, Component> components;
			std::map<UUID, SchematicSymbol> symbols;
			std::map<UUID, SymbolPin> pins;
			std::map<UUID, Net> nets;
			std::map<UUID, Hole> holes;

			json serialize();

		private:
			Cores core;
	};
}
