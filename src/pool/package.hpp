#pragma once
#include "util/uuid.hpp"
#include "json.hpp"
#include "common/common.hpp"
#include "util/uuid_provider.hpp"
#include "common/junction.hpp"
#include "common/line.hpp"
#include "common/polygon.hpp"
#include "common/hole.hpp"
#include "common/arc.hpp"
#include "common/text.hpp"
#include "package/pad.hpp"
#include "util/warning.hpp"
#include "common/layer_provider.hpp"
#include <vector>
#include <map>
#include <fstream>
#include <set>
#include "common/object_provider.hpp"
#include "package/package_rules.hpp"

namespace horizon {
	using json = nlohmann::json;


	class Package : public ObjectProvider, public LayerProvider, public UUIDProvider {
		public :
			class MyParameterProgram: public ParameterProgram {
				friend Package;
				private:
					ParameterProgram::CommandHandler get_command(const std::string &cmd) override;
					class Package *pkg = nullptr;

					std::pair<bool, std::string> set_polygon(const ParameterProgram::TokenCommand *cmd, std::deque<int64_t> &stack);
					std::pair<bool, std::string> expand_polygon(const ParameterProgram::TokenCommand *cmd, std::deque<int64_t> &stack);

				public:
					MyParameterProgram(class Package *p, const std::string &code);


			};

			Package(const UUID &uu, const json &j, class Pool &pool);
			Package(const UUID &uu);
			static Package new_from_file(const std::string &filename, class Pool &pool);

			json serialize() const ;
			virtual Junction *get_junction(const UUID &uu);
			std::pair<Coordi, Coordi> get_bbox() const;
			const std::map<int, Layer> &get_layers() const override;
			std::pair<bool, std::string> apply_parameter_set(const ParameterSet &ps);

			UUID get_uuid() const;

			Package(const Package &pkg);
			void operator=(Package const &pkg);

			UUID uuid;
			std::string name;
			std::string manufacturer;
			std::set<std::string> tags;

			std::map<UUID, Junction> junctions;
			std::map<UUID, Line> lines;
			std::map<UUID, Arc> arcs;
			std::map<UUID, Text> texts;
			std::map<UUID, Pad> pads;
			std::map<UUID, Polygon> polygons;

			ParameterSet parameter_set;
			MyParameterProgram parameter_program;
			PackageRules rules;

			std::string model_filename;
			const class Package *alternate_for = nullptr;

			std::vector<Warning> warnings;

		private :
			void update_refs();
	};
}
