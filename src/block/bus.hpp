#pragma once
#include "util/uuid.hpp"
#include "json.hpp"
#include "pool/unit.hpp"
#include "util/uuid_provider.hpp"
#include "util/uuid_ptr.hpp"
#include <deque>
#include <map>
#include <fstream>
#include "block/net.hpp"

namespace horizon {
	using json = nlohmann::json;

	/**
	 * A Bus is used for grouping nets.
	 * A Net becomes member of a Bus by creating a Bus::Member
	 * for the Net. The member's name is independent from
	 * the the Net's name.
	 */
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
