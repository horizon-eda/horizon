#pragma once
#include "uuid.hpp"
#include "package.hpp"
#include "entity.hpp"
#include <vector>
#include <map>
#include <fstream>

namespace horizon {
	using json = nlohmann::json;

	class Part {
		private :
			Part(const UUID &uu, const json &j, Pool &pool);
			const std::string empty;

		public :
			class PadMapItem {
				public:
				PadMapItem(Gate *g, Pin *p): gate(g), pin(p) {}
				Gate *gate;
				Pin *pin;
			};
			Part(const UUID &uu);

			static Part new_from_file(const std::string &filename, Pool &pool);
			UUID uuid;


			enum class Attribute {MPN, VALUE, MANUFACTURER};
			std::map<Attribute, std::pair<bool, std::string>> attributes;
			const std::string &get_attribute(Attribute a) const;
			const std::pair<bool, std::string> &get_attribute_pair(Attribute a) const;

			const std::string &get_MPN() const;
			const std::string &get_value() const;
			const std::string &get_manufacturer() const;
			std::set<std::string> get_tags() const;

			std::set<std::string> tags;
			bool inherit_tags = false;
			Entity *entity = nullptr;
			Package *package = nullptr;
			class Part *base = nullptr;

			std::map<std::string, std::string> parametric;

			std::map<UUID, PadMapItem> pad_map;
			json serialize() const;
	};

}
