#pragma once
#include "uuid.hpp"
#include "json.hpp"
#include "common.hpp"
#include "uuid_ptr.hpp"
#include "junction.hpp"
#include "padstack.hpp"
#include "via_template.hpp"
#include <vector>
#include <map>
#include <set>
#include <fstream>


namespace horizon {
	using json = nlohmann::json;

	class Via {
		public :
			Via(const UUID &uu, const json &j, class Board &brd);
			Via(const UUID &uu, class ViaTemplate *vt);

			UUID uuid;

			uuid_ptr<Junction> junction = nullptr;
			uuid_ptr<class ViaTemplate> via_template;
			Padstack padstack;


			json serialize() const;
	};
}
