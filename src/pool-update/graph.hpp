#pragma once
#include "util/uuid.hpp"
#include <string>
#include <set>
#include <map>
#include <vector>

namespace horizon {

	class PoolUpdateNode {
		public:
			PoolUpdateNode(const UUID &uu, const std::string &filename, const std::set<UUID> &dependencies);

			const UUID uuid;
			const std::string filename;

			std::set<UUID> dependencies;
			std::set<class PoolUpdateNode*> dependants;
	};

	class PoolUpdateGraph {
		public:
			PoolUpdateGraph();
			void add_node(const UUID &uu, const std::string &filename, const std::set<UUID> &dependencies);
			void dump(const std::string &filename);
			std::set<std::pair<const PoolUpdateNode*, UUID>> update_dependants();

			const PoolUpdateNode &get_root() const;

		private:
			std::map<UUID, PoolUpdateNode> nodes;
			PoolUpdateNode root_node;

	};
}
