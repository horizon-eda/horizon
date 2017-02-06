#pragma once
#include "uuid.hpp"
#include "json_fwd.hpp"
#include "common.hpp"
#include "unit.hpp"
#include "junction.hpp"
#include "line.hpp"
#include "arc.hpp"
#include "object.hpp"
#include "text.hpp"
#include "uuid_provider.hpp"
#include <vector>
#include <map>
#include <fstream>
#include <set>

namespace horizon {
	using json = nlohmann::json;
	
	
	class SymbolPin: public UUIDProvider {
		public :
			enum class ConnectorStyle {BOX, NONE};

			SymbolPin(const UUID &uu, const json &j);
			SymbolPin(UUID uu);

			const UUID uuid;
			Coord<int64_t> position;
			uint64_t length;
			bool name_visible;
			bool pad_visible;
			Orientation orientation;
			
			//not stored
			std::string name;
			std::string pad;
			ConnectorStyle connector_style = ConnectorStyle::BOX;
			std::map<UUID, class LineNet*> connected_net_lines;
			UUID net_segment;


			json serialize() const;
			virtual UUID get_uuid() const;
	};
	
	class Symbol : public Object {
		public :
			Symbol(const UUID &uu, const json &j, Object &obj);
			Symbol(const UUID &uu);
			static Symbol new_from_file(const std::string &filename, Object &obj);
			std::pair<Coordi, Coordi> get_bbox(bool all=false) const;
			virtual Junction *get_junction(const UUID &uu);
			virtual SymbolPin *get_symbol_pin(const UUID &uu);
			
			json serialize() const ;

			void expand();
			Symbol(const Symbol &sym);
			void operator=(Symbol const &sym);

			UUID uuid;
			Unit *unit;
			std::string name;
			std::map<UUID, SymbolPin> pins;
			std::map<UUID, Junction> junctions;
			std::map<UUID, Line> lines;
			std::map<UUID, Arc> arcs;
			std::map<UUID, Text> texts;
			
		private :
			void update_refs();
	};
}
