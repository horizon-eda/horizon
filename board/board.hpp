#pragma once
#include "uuid.hpp"
#include "json.hpp"
#include "pool.hpp"
#include "block.hpp"
#include "polygon.hpp"
#include "hole.hpp"
#include "board_package.hpp"
#include "junction.hpp"
#include "track.hpp"
#include "via_padstack_provider.hpp"
#include "via.hpp"
#include "clipper/clipper.hpp"
#include "warning.hpp"
#include <vector>
#include <map>
#include <fstream>

namespace horizon {
	using json = nlohmann::json;

	class Board {
		private :
			Board(const UUID &uu, const json &, Block &block, Pool &pool, ViaPadstackProvider &vpp);
			//unsigned int update_nets();
			bool propagate_net_segments();
			std::map<UUID, uuid_ptr<Net>> net_segments;


		public :
			static Board new_from_file(const std::string &filename, Block &block, Pool &pool, ViaPadstackProvider &vpp);
			Board(const UUID &uu, Block &block);

			void expand(bool careful=false);

			Board(const Board &brd);
			void operator=(const Board &brd);
			void update_refs();
			void update_airwires();
			void disconnect_package(BoardPackage *pkg);
			void delete_dependants();
			void vacuum_junctions();
			void smash_package(BoardPackage *pkg);
			void unsmash_package(BoardPackage *pkg);

			UUID uuid;
			Block *block;
			std::string name;
			unsigned int n_inner_layers = 0;
			std::map<UUID, Polygon> polygons;
			std::map<UUID, Hole> holes;
			std::map<UUID, BoardPackage> packages;
			std::map<UUID, Junction> junctions;
			std::map<UUID, Track> tracks;
			std::map<UUID, Track> airwires;
			std::map<UUID, Via> vias;
			std::map<UUID, Text> texts;

			std::vector<Warning> warnings;

			ClipperLib::Paths obstacles;
			ClipperLib::Path  track_path;

			json serialize() const;
	};

}
