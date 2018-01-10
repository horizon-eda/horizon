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
#include "component.hpp"
#include "schematic_symbol.hpp"
#include "symbol.hpp"
#include "hole.hpp"
#include "polygon.hpp"
#include "net.hpp"
#include "shape.hpp"
#include "net_label.hpp"
#include "power_symbol.hpp"
#include "json.hpp"
#include "canvas/selectables.hpp"

namespace horizon {
	class Buffer {
		public:
			Buffer(class Core *co);
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
			std::map<UUID, LineNet> net_lines;
			std::map<UUID, Hole> holes;
			std::map<UUID, Shape> shapes;
			std::map<UUID, PowerSymbol> power_symbols;
			std::map<UUID, NetLabel> net_labels;

			json serialize();

		private:
			Cores core;
			NetClass net_class_dummy;
	};
}
