#pragma once
#include "uuid.hpp"
#include "json.hpp"
#include "common.hpp"
#include "object.hpp"
#include "uuid_provider.hpp"
#include "junction.hpp"
#include "line.hpp"
#include "polygon.hpp"
#include "hole.hpp"
#include "arc.hpp"
#include "text.hpp"
#include "pad.hpp"
#include "warning.hpp"
#include "layer_provider.hpp"
#include <vector>
#include <map>
#include <fstream>
#include <set>

namespace horizon {
	using json = nlohmann::json;


	class Package : public Object, public LayerProvider {
		public :
			Package(const UUID &uu, const json &j, class Pool &pool);
			Package(const UUID &uu);
			static Package new_from_file(const std::string &filename, class Pool &pool);

			json serialize() const ;
			virtual Junction *get_junction(const UUID &uu);
			std::pair<Coordi, Coordi> get_bbox() const;
			const std::map<int, Layer> &get_layers() const override;

			Package(const Package &pkg);
			void operator=(Package const &pkg);

			UUID uuid;
			std::string name;
			std::set<std::string> tags;

			std::map<UUID, Junction> junctions;
			std::map<UUID, Line> lines;
			std::map<UUID, Arc> arcs;
			std::map<UUID, Text> texts;
			std::map<UUID, Pad> pads;
			std::map<UUID, Polygon> polygons;
			std::vector<Warning> warnings;

		private :
			void update_refs();
	};
}
