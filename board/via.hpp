#pragma once
#include "uuid.hpp"
#include "json.hpp"
#include "common.hpp"
#include "uuid_ptr.hpp"
#include "junction.hpp"
#include "padstack.hpp"
#include <vector>
#include <map>
#include <set>
#include <fstream>


namespace horizon {
	using json = nlohmann::json;

	class Via {
		public :
			Via(const UUID &uu, const json &j, class Board &brd, class ViaPadstackProvider &vpp);
			Via(const UUID &uu, Padstack *ps);

			UUID uuid;

			uuid_ptr<Junction> junction = nullptr;
			Padstack *vpp_padstack = nullptr;
			Padstack padstack;


			json serialize() const;
	};
}
