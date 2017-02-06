#pragma once
#include "uuid.hpp"
#include "json_fwd.hpp"
#include "object.hpp"
#include "unit.hpp"
#include "uuid_provider.hpp"
#include "uuid_ptr.hpp"
#include <deque>
#include <map>
#include <fstream>

namespace horizon {
	using json = nlohmann::json;

	class Bus :public UUIDProvider{
		public :
			class Member: public UUIDProvider {
				public:
				Member(const UUID &uu, const json &, class Block &block);
				Member(const UUID &uu);
				UUID uuid;
				std::string name;
				uuid_ptr<Net> net = nullptr;
				json serialize() const;
				virtual UUID get_uuid() const;

			};


			Bus(const UUID &uu, const json &, class Block &block);
			Bus(const UUID &uu);
			virtual UUID get_uuid() const;
			UUID uuid;
			std::string name;
			std::map<UUID, Member> members;
			bool is_referenced = false;
			void update_refs(Block &block);
			json serialize() const;
	};

}
