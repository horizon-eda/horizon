#pragma once
#include "util/uuid.hpp"
#include "json.hpp"
#include "common/common.hpp"
#include "common/layer_provider.hpp"
#include "unit.hpp"
#include "common/junction.hpp"
#include "common/line.hpp"
#include "common/arc.hpp"
#include "common/text.hpp"
#include "util/uuid_provider.hpp"
#include "common/polygon.hpp"
#include <vector>
#include <map>
#include <fstream>
#include <set>
#include "common/object_provider.hpp"

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
			Orientation get_orientation_for_placement(const Placement &p) const;
			
			class Decoration {
				public:
					Decoration();
					Decoration(const json &j);
					bool dot = false;
					bool clock = false;
					bool schmitt = false;
					enum class Driver {DEFAULT, OPEN_COLLECTOR, OPEN_COLLECTOR_PULLUP, OPEN_EMITTER, OPEN_EMITTER_PULLDOWN, TRISTATE};
					Driver driver = Driver::DEFAULT;

					json serialize() const;
			};
			Decoration decoration;

			//not stored
			std::string name;
			std::string pad;
			ConnectorStyle connector_style = ConnectorStyle::BOX;
			std::map<UUID, class LineNet*> connected_net_lines;
			UUID net_segment;
			Pin::Direction direction = Pin::Direction::BIDIRECTIONAL;


			json serialize() const;
			virtual UUID get_uuid() const;
	};
	
	class Symbol : public ObjectProvider, public LayerProvider {
		public :
			Symbol(const UUID &uu, const json &j, class Pool &pool);
			Symbol(const UUID &uu);
			static Symbol new_from_file(const std::string &filename, Pool &pool);
			std::pair<Coordi, Coordi> get_bbox(bool all=false) const;
			virtual Junction *get_junction(const UUID &uu);
			virtual SymbolPin *get_symbol_pin(const UUID &uu);
			
			json serialize() const ;

			/**
			 * fills in information from the referenced unit
			 */
			void expand();
			Symbol(const Symbol &sym);
			void operator=(Symbol const &sym);

			UUID uuid;
			uuid_ptr<const Unit> unit;
			std::string name;
			std::map<UUID, SymbolPin> pins;
			std::map<UUID, Junction> junctions;
			std::map<UUID, Line> lines;
			std::map<UUID, Arc> arcs;
			std::map<UUID, Text> texts;
			std::map<UUID, Polygon> polygons;
			
			std::map<std::tuple<int, bool, UUID>, Placement> text_placements;
			void apply_placement(const Placement &p);

		private :
			void update_refs();
	};
}
