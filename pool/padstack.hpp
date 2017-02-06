#pragma once
#include "uuid.hpp"
#include "json_fwd.hpp"
#include "common.hpp"
#include "uuid_provider.hpp"
#include "polygon.hpp"
#include "hole.hpp"
#include "lut.hpp"
#include <vector>
#include <map>
#include <fstream>
#include <set>

namespace horizon {
	using json = nlohmann::json;


	class Padstack : public UUIDProvider {
		public :
			enum class Type {TOP, BOTTOM, THROUGH};
			static const LutEnumStr<Padstack::Type> type_lut;

			Padstack(const UUID &uu, const json &j);
			Padstack(const UUID &uu);
			static Padstack new_from_file(const std::string &filename);

			json serialize() const ;

			UUID uuid;
			std::string name;
			Type type = Type::TOP;
			std::map<UUID, Polygon> polygons;
			std::map<UUID, Hole> holes;
			UUID get_uuid() const override;
			std::pair<Coordi, Coordi> get_bbox() const;
			void expand_inner(unsigned int n_inner);

		private :
			void update_refs();
	};
}
