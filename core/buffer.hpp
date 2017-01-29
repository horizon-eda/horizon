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
#include "json_fwd.hpp"

namespace horizon {
	class Buffer {
		public:
			Buffer(Core *co);
			void clear();
			void load_from_symbol(std::set<SelectableRef> selection);

			std::map<const UUID, Text> texts;
			std::map<const UUID, Junction> junctions;
			std::map<const UUID, Line> lines;
			std::map<const UUID, Arc> arcs;
			std::map<const UUID, Pad> pads;
			std::map<const UUID, Polygon> polygons;
			std::map<const UUID, Component> components;
			std::map<const UUID, SchematicSymbol> symbols;
			std::map<const UUID, SymbolPin> pins;
			std::map<const UUID, Net> nets;
			std::map<const UUID, Hole> holes;

			json serialize();

		private:
			Cores core;
	};
}
