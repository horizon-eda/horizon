#pragma once
#include "uuid.hpp"
#include "json_fwd.hpp"
#include "common.hpp"
#include "unit.hpp"
#include "entity.hpp"
#include "object.hpp"
#include "symbol.hpp"
#include "padstack.hpp"
#include <vector>
#include <map>
#include <fstream>
#include <sqlite3.h>

namespace horizon {

	class ViaPadstackProvider {
		public :
			ViaPadstackProvider(const std::string &p);
			Padstack *get_padstack(const UUID &uu);
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
			std::map<UUID, Padstack> padstacks;
			std::map<UUID, PadstackEntry> padstacks_available;


	};

}
