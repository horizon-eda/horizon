#pragma once
#include "util/uuid.hpp"
#include "json.hpp"
#include "pool/pool.hpp"
#include "block/block.hpp"
#include "common/polygon.hpp"
#include "common/hole.hpp"
#include "board_package.hpp"
#include "common/junction.hpp"
#include "board/track.hpp"
#include "board/via_padstack_provider.hpp"
#include "board/via.hpp"
#include "board/plane.hpp"
#include "clipper/clipper.hpp"
#include "util/warning.hpp"
#include "board_rules.hpp"
#include "common/dimension.hpp"
#include <vector>
#include <map>
#include <fstream>
#include "common/layer_provider.hpp"
#include "board/fab_output_settings.hpp"
#include "board/board_hole.hpp"

namespace horizon {
	using json = nlohmann::json;

	class Board: public ObjectProvider, public LayerProvider {
		private :
			Board(const UUID &uu, const json &, Block &block, Pool &pool, ViaPadstackProvider &vpp);
			//unsigned int update_nets();
			bool propagate_net_segments();
			std::map<UUID, uuid_ptr<Net>> net_segments;
			std::map<int, Layer> layers;

			void delete_dependants();
			void vacuum_junctions();


		public :
			static Board new_from_file(const std::string &filename, Block &block, Pool &pool, ViaPadstackProvider &vpp);
			Board(const UUID &uu, Block &block);

			void expand(bool careful=false);
			void expand_packages();

			Board(const Board &brd);
			void operator=(const Board &brd);
			void update_refs();
			void update_airwires(bool fast=false);
			void disconnect_package(BoardPackage *pkg);

			void smash_package(BoardPackage *pkg);
			void unsmash_package(BoardPackage *pkg);
			Junction *get_junction(const UUID &uu) override;
			const std::map<int, Layer> &get_layers() const override;
			void set_n_inner_layers(unsigned int n);
			unsigned int get_n_inner_layers() const;
			void update_plane(Plane *plane, class CanvasPatch *ca=nullptr, class CanvasPads *ca_pads=nullptr); //when ca is given, patches will be read from it
			void update_planes();

			UUID uuid;
			Block *block;
			std::string name;
			std::map<UUID, Polygon> polygons;
			std::map<UUID, BoardHole> holes;
			std::map<UUID, BoardPackage> packages;
			std::map<UUID, Junction> junctions;
			std::map<UUID, Track> tracks;
			std::map<UUID, Track> airwires;
			std::map<UUID, Via> vias;
			std::map<UUID, Text> texts;
			std::map<UUID, Line> lines;
			std::map<UUID, Arc> arcs;
			std::map<UUID, Plane> planes;
			std::map<UUID, Dimension> dimensions;

			std::vector<Warning> warnings;

			BoardRules rules;
			FabOutputSettings fab_output_settings;

			class StackupLayer {
				public:
					StackupLayer(int l, const json &j);
					StackupLayer(int l);
					json serialize() const;
					int layer;
					uint64_t thickness = 0.035_mm;
					uint64_t substrate_thickness = .1_mm;
			};
			std::map<int, StackupLayer> stackup;


			ClipperLib::Paths obstacles;
			ClipperLib::Path  track_path;

			json serialize() const;

		private:
			unsigned int n_inner_layers = 0;
			ClipperLib::Paths get_thermals(class Plane *plane, class CanvasPads *ca) const;
	};

}
