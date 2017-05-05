#pragma once
#include "uuid.hpp"
#include "json.hpp"
#include "common.hpp"
#include "junction.hpp"
#include "object.hpp"
#include "uuid_provider.hpp"
#include "uuid_ptr.hpp"
#include "board_package.hpp"
#include "net.hpp"
#include <vector>
#include <map>
#include <fstream>
		

namespace horizon {
	using json = nlohmann::json;
	
	
	class Track : public UUIDProvider{
		public :
			enum class End {TO, FROM};

			Track(const UUID &uu, const json &j, class Board &brd);
			Track(const UUID &uu);

			void update_refs(class Board  &brd);
			virtual UUID get_uuid() const;
			bool coord_on_line(const Coordi &coord) const;

			UUID uuid;
			uuid_ptr<Net> net=nullptr;
			UUID net_segment = UUID();
			int layer = 0;
			uint64_t width = 0;
			bool width_from_rules = true;
			bool is_air = false;



			class Connection {
				public:
					Connection() {}
					Connection(const json &j, Board &brd);
					Connection(Junction *j);
					Connection(BoardPackage *pkg, Pad *pad);
					uuid_ptr<Junction> junc = nullptr;
					uuid_ptr<BoardPackage> package = nullptr;
					uuid_ptr<Pad> pad = nullptr;

					void connect(Junction *j);
					void connect(BoardPackage *pkg, Pad *pad);
					UUIDPath<2> get_pad_path() const ;
					bool is_junc() const ;
					bool is_pad() const ;
					UUID get_net_segment() const;
					void update_refs(class Board &brd);
					Coordi get_position() const;
					json serialize() const;
			};

			Connection from;
			Connection to;


			json serialize() const;
	};
}
