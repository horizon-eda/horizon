#pragma once
#include "util/uuid.hpp"
#include "json.hpp"
#include <map>
#include <deque>

namespace horizon {
	using json = nlohmann::json;

	class ProjectBlock {
		public:
			ProjectBlock(const UUID &uu, const std::string &b, const std::string &s, bool t=false): uuid(uu), block_filename(b), schematic_filename(s), is_top(t) {}
			UUID uuid;
			std::string block_filename;
			std::string schematic_filename;
			bool is_top;
	};

	class Project {
		private :
			Project(const UUID &uu, const json &, const std::string &base);
			std::string get_filename_rel(const std::string &p) const;


		public :
			static Project new_from_file(const std::string &filename);
			Project(const UUID &uu);
			ProjectBlock &get_top_block();

			std::string create(const UUID &default_via = UUID());

			std::string base_path;
			UUID uuid;
			std::string name;
			std::string title;
			std::map<UUID, ProjectBlock> blocks;

			UUID pool_uuid;
			std::string vias_directory;
			std::string board_filename;
			std::string pool_cache_directory;

			json serialize() const;
	};

}
