#pragma once
#include "util/uuid.hpp"
#include "json.hpp"
#include "common/common.hpp"
#include "common/junction.hpp"
#include "util/uuid_provider.hpp"
#include "util/uuid_ptr.hpp"
#include "schematic/schematic_symbol.hpp"
#include "block/net.hpp"
#include "block/bus.hpp"
#include "schematic/power_symbol.hpp"
#include "schematic/bus_ripper.hpp"
#include <vector>
#include <map>
#include <fstream>

namespace horizon {
	using json = nlohmann::json;
	
	/**
	 * LineNet is similar to Line, except it denotes electrical connection.
	 * When connected to a BusLabel, it denotes a Bus.
	 */
	class LineNet : public UUIDProvider{
		public :
			enum class End {TO, FROM};

			LineNet(const UUID &uu, const json &j, class Sheet *sheet=nullptr);
			LineNet(const UUID &uu);

			void update_refs(class Sheet &sheet);
			bool is_connected_to(const UUID &uu_sym, const UUID &uu_pin) const ;
			virtual UUID get_uuid() const;
			bool coord_on_line(const Coordi &coord) const;

			uuid_ptr<Net> net=nullptr;
			uuid_ptr<Bus> bus=nullptr;
			UUID net_segment = UUID();


			const UUID uuid;

			class Connection {
				public:
					Connection() {}
					Connection(const json &j, Sheet *sheet);
					uuid_ptr<Junction> junc = nullptr;
					uuid_ptr<SchematicSymbol> symbol = nullptr;
					uuid_ptr<SymbolPin> pin = nullptr;
					uuid_ptr<BusRipper> bus_ripper = nullptr;
					bool operator<(const Connection &other) const;
					bool operator==(const Connection &other) const;

					void connect(Junction *j);
					void connect(BusRipper *r);
					void connect(SchematicSymbol *j, SymbolPin *pin);
					UUIDPath<2> get_pin_path() const ;
					bool is_junc() const ;
					bool is_pin() const ;
					bool is_bus_ripper() const;
					UUID get_net_segment() const;
					void update_refs(class Sheet &sheet);
					Coordi get_position() const;
					json serialize() const;
			};

			Connection from;
			Connection to;


			json serialize() const;
	};
}
