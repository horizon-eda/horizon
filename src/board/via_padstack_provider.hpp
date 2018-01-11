#pragma once
#include "util/uuid.hpp"
#include "json.hpp"
#include "common/common.hpp"
#include "pool/padstack.hpp"
#include <vector>
#include <map>
#include <fstream>
#include <sqlite3.h>

namespace horizon {

	class ViaPadstackProvider {
		public :
			ViaPadstackProvider(const std::string &p, class Pool &po);
			const Padstack *get_padstack(const UUID &uu);
			void update_available();
			class PadstackEntry{
				public:
					PadstackEntry(const std::string &p, const std::string &n):
						path(p), name(n)
					{}

					std::string path;
					std::string name;
			};
			const std::map<UUID, PadstackEntry> &get_padstacks_available() const;

		private :
			std::string base_path;
			class Pool &pool;

			std::map<UUID, Padstack> padstacks;
			std::map<UUID, PadstackEntry> padstacks_available;


	};

}
