#pragma once
#include "uuid.hpp"
#include "json.hpp"
#include "common.hpp"
#include "uuid_ptr.hpp"
#include <vector>
#include <map>
#include <set>
#include <fstream>
#include "padstack.hpp"
#include "uuid_provider.hpp"


namespace horizon {
	using json = nlohmann::json;

	class ViaTemplate: public UUIDProvider {
		public :
			ViaTemplate(const UUID &uu, const json &j, class ViaPadstackProvider &vpp);
			ViaTemplate(const UUID &uu, const Padstack *ps);
			UUID get_uuid() const;

			UUID uuid;
			std::string name;

			const Padstack *padstack = nullptr;
			ParameterSet parameter_set;

			bool is_referenced = false;

			json serialize() const;
	};
}
