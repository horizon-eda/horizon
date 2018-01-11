#include "pool/package.hpp"
#include "pool/pool.hpp"
#include "util/util.hpp"
#include "clipper/clipper.hpp"

namespace horizon {

	Package::MyParameterProgram::MyParameterProgram(Package *p, const std::string &c): ParameterProgram(c), pkg(p) {}

	static bool stack_pop(std::deque<int64_t> &stack, int64_t &va) {
		if(stack.size()) {
			va = stack.back();
			stack.pop_back();
			return false;
		}
		else {
			return true;
		}
	}

	std::pair<bool, std::string> Package::MyParameterProgram::set_polygon(const ParameterProgram::TokenCommand *cmd, std::deque<int64_t> &stack) {
		if(cmd->arguments.size()<4 ||
		   cmd->arguments.at(0)->type != ParameterProgram::Token::Type::STR ||
		   cmd->arguments.at(1)->type != ParameterProgram::Token::Type::STR ||
		   cmd->arguments.at(2)->type != ParameterProgram::Token::Type::INT ||
		   cmd->arguments.at(3)->type != ParameterProgram::Token::Type::INT)
			return {true, "not enough arguments"};

		auto pclass = dynamic_cast<ParameterProgram::TokenString*>(cmd->arguments.at(0).get())->string;
		auto shape = dynamic_cast<ParameterProgram::TokenString*>(cmd->arguments.at(1).get())->string;
		auto x0 = dynamic_cast<ParameterProgram::TokenInt*>(cmd->arguments.at(2).get())->value;
		auto y0 = dynamic_cast<ParameterProgram::TokenInt*>(cmd->arguments.at(3).get())->value;

		if(shape == "rectangle") {
			int64_t width,height;
			if(stack_pop(stack, height) || stack_pop(stack, width))
				return {true, "empty stack"};
			for(auto &it: pkg->polygons) {
				if(it.second.parameter_class == pclass) {
					it.second.vertices = {
						Coordi(x0-width/2, y0-height/2),
						Coordi(x0-width/2, y0+height/2),
						Coordi(x0+width/2, y0+height/2),
						Coordi(x0+width/2, y0-height/2),
					};
				}
			}
		}

		else {
			return {true, "unknown shape "+shape};
		}

		return {false, ""};
	}

	std::pair<bool, std::string> Package::MyParameterProgram::expand_polygon(const ParameterProgram::TokenCommand *cmd, std::deque<int64_t> &stack) {
		if(cmd->arguments.size()<1 || cmd->arguments.at(0)->type != ParameterProgram::Token::Type::STR)
			return {true, "not enough arguments"};

		if(!(cmd->arguments.size()&1)) {
			return {true, "number of coordinates must be even"};
		}
		ClipperLib::Path path;
		for(size_t i = 0; i<cmd->arguments.size()-1; i+=2) {
			if(cmd->arguments.at(i+1)->type != ParameterProgram::Token::Type::INT || cmd->arguments.at(i+2)->type != ParameterProgram::Token::Type::INT) {
				return {true, "coordinates must be int"};
			}
			auto x = dynamic_cast<ParameterProgram::TokenInt*>(cmd->arguments.at(i+1).get())->value;
			auto y = dynamic_cast<ParameterProgram::TokenInt*>(cmd->arguments.at(i+2).get())->value;
			path.emplace_back(ClipperLib::IntPoint(x, y));
		}
		if(path.size()<3) {
			return {true, "must have at least 3 vertices"};
		}

		int64_t expand;
		if(stack_pop(stack, expand))
			return {true, "empty stack"};

		ClipperLib::ClipperOffset ofs;
		ofs.AddPath(path, ClipperLib::jtMiter, ClipperLib::etClosedPolygon);
		ClipperLib::Paths paths_expanded;
		ofs.Execute(paths_expanded, expand);
		if(paths_expanded.size() != 1) {
			return {true, "expand error"};
		}

		auto pclass = dynamic_cast<ParameterProgram::TokenString*>(cmd->arguments.at(0).get())->string;

		for(auto &it: pkg->polygons) {
			if(it.second.parameter_class == pclass) {
				it.second.vertices.clear();
				for(const auto &c: paths_expanded[0]) {
					it.second.vertices.emplace_back(Coordi(c.X, c.Y));
				}
			}
		}

		return {false, ""};
	}

	ParameterProgram::CommandHandler Package::MyParameterProgram::get_command(const std::string &cmd) {
		using namespace std::placeholders;
		if(auto r = ParameterProgram::get_command(cmd)) {
			return r;
		}
		else if(cmd == "set-polygon") {
			return std::bind(std::mem_fn(&Package::MyParameterProgram::set_polygon), this, _1, _2);
		}
		else if(cmd == "expand-polygon") {
			return std::bind(std::mem_fn(&Package::MyParameterProgram::expand_polygon), this, _1, _2);
		}
		return nullptr;
	}

	Package::Package(const UUID &uu, const json &j, Pool &pool):
			uuid(uu),
			name(j.at("name").get<std::string>()),
			manufacturer(j.value("manufacturer", "")),
			parameter_program(this, j.value("parameter_program", "")),
			model_filename(j.value("model_filename", ""))
		{
		{
			const json &o = j["junctions"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID(it.key());
				junctions.emplace(std::make_pair(u, Junction(u, it.value())));
			}
		}
		{
			const json &o = j["lines"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID(it.key());
				lines.emplace(std::make_pair(u, Line(u, it.value(), *this)));
			}
		}
		{
			const json &o = j["arcs"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID(it.key());
				arcs.emplace(std::make_pair(u, Arc(u, it.value(), *this)));
			}
		}
		{
			const json &o = j["texts"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID(it.key());
				texts.emplace(std::make_pair(u, Text(u, it.value())));
			}
		}
		{
			const json &o = j["pads"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID(it.key());
				pads.emplace(std::make_pair(u, Pad(u, it.value(), pool)));
			}
		}

		if(j.count("polygons")){
			const json &o = j["polygons"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID(it.key());
				polygons.emplace(std::make_pair(u, Polygon(u, it.value())));
			}
		}
		map_erase_if(polygons, [](const auto &a){return a.second.vertices.size()==0;});
		if(j.count("tags")) {
			tags = j.at("tags").get<std::set<std::string>>();
		}
		if(j.count("parameter_set")) {
			parameter_set = parameter_set_from_json(j.at("parameter_set"));
		}
		if(j.count("alternate_for")) {
			alternate_for = pool.get_package(UUID(j.at("alternate_for").get<std::string>()));
		}
	}

	Package Package::new_from_file(const std::string &filename, Pool &pool) {
		json j;
		std::ifstream ifs(filename);
		if(!ifs.is_open()) {
			throw std::runtime_error("file "  +filename+ " not opened");
		}
		ifs>>j;
		ifs.close();
		return Package(UUID(j["uuid"].get<std::string>()), j, pool);
	}

	Package::Package(const UUID &uu): uuid(uu), parameter_program(this, "") {}

	Junction *Package::get_junction(const UUID &uu) {
		return &junctions.at(uu);
	}

	Package::Package(const Package &pkg):
		uuid(pkg.uuid),
		name(pkg.name),
		manufacturer(pkg.manufacturer),
		tags(pkg.tags),
		junctions(pkg.junctions),
		lines(pkg.lines),
		arcs(pkg.arcs),
		texts(pkg.texts),
		pads(pkg.pads),
		polygons(pkg.polygons),
		parameter_set(pkg.parameter_set),
		parameter_program(pkg.parameter_program),
		model_filename(pkg.model_filename),
		alternate_for(pkg.alternate_for),
		warnings(pkg.warnings)
	{
		update_refs();
	}

	void Package::operator=(Package const &pkg) {
		uuid = pkg.uuid;
		name = pkg.name;
		manufacturer = pkg.manufacturer;
		tags = pkg.tags;
		junctions = pkg.junctions;
		lines = pkg.lines;
		arcs = pkg.arcs;
		texts = pkg.texts;
		pads = pkg.pads;
		polygons = pkg.polygons;
		parameter_set = pkg.parameter_set;
		parameter_program = pkg.parameter_program;
		model_filename = pkg.model_filename;
		alternate_for = pkg.alternate_for;
		warnings = pkg.warnings;
		update_refs();
	}

	void Package::update_refs() {
		for(auto &it: lines) {
			auto &line = it.second;
			line.to = &junctions.at(line.to.uuid);
			line.from = &junctions.at(line.from.uuid);
		}
		for(auto &it: arcs) {
			auto &arc = it.second;
			arc.to = &junctions.at(arc.to.uuid);
			arc.from = &junctions.at(arc.from.uuid);
			arc.center = &junctions.at(arc.center.uuid);
		}
		parameter_program.pkg = this;
	}

	static void copy_param(ParameterSet &dest, const ParameterSet &src, ParameterID id) {
		if(src.count(id))
			dest[id] = src.at(id);
	}

	static void copy_param(ParameterSet &dest, const ParameterSet &src, const std::set<ParameterID> &ids) {
		for(const auto id: ids) {
			copy_param(dest, src, id);
		}
	}

	std::pair<bool, std::string> Package::apply_parameter_set(const ParameterSet &ps) {
		auto ps_this = parameter_set;
		copy_param(ps_this, ps, ParameterID::COURTYARD_EXPANSION);
		auto r = parameter_program.run(ps_this);
		if(r.first) {
			return r;
		}

		for(auto &it: pads) {
			auto ps_pad = it.second.parameter_set;
			copy_param(ps_pad, ps, {ParameterID::SOLDER_MASK_EXPANSION, ParameterID::PASTE_MASK_CONTRACTION, ParameterID::HOLE_SOLDER_MASK_EXPANSION});
			auto r = it.second.padstack.apply_parameter_set(ps_pad);
			if(r.first) {
				return r;
			}
		}
		return {false, ""};
	}

	std::pair<Coordi, Coordi> Package::get_bbox() const {
		Coordi a;
		Coordi b;
		for(const auto &it: pads) {
			auto bb_pad = it.second.placement.transform_bb(it.second.padstack.get_bbox());
			a = Coordi::min(a, bb_pad.first);
			b = Coordi::max(b, bb_pad.second);
		}
		return std::make_pair(a,b);
	}

	const std::map<int, Layer> &Package::get_layers() const {
			static const std::map<int, Layer> layers = {
			{60, {60, "Top Courtyard", {.5,.5,.5}}},
			{50, {50, "Top Assembly", {.5,.5,.5}}},
			{40, {40, "Top Package", {.5,.5,.5}}},
			{30, {30, "Top Paste", {.8,.8,.8}}},
			{20, {20, "Top Silkscreen", {.9,.9,.9}}},
			{10, {10, "Top Mask", {1,.5,.5}}},
			{0, {0, "Top Copper", {1,0,0}, false, true}},
			{-1, {-1, "Inner", {1,1,0}, false, true}},
			{-100, {-100, "Bottom Copper", {0,.5,0}, true, true}},
			{-110, {-110, "Bottom Mask", {.25,.5,.25}, true}},
			{-120, {-120, "Bottom Silkscreen", {.9,.9,.9}, true}},
			{-130, {-130, "Bottom Paste", {.8,.8,.8}}},
			{-140, {-140, "Bottom Package", {.5,.5,.5}}},
			{-150, {-150, "Bottom Assembly", {.5,.5,.5}, true}},
			{-160, {-160, "Bottom Courtyard", {.5,.5,.5}}}
		};
		return layers;
	}


	json Package::serialize() const {
		json j;
		j["uuid"] = (std::string)uuid;
		j["type"] = "package";
		j["name"] = name;
		j["manufacturer"] = manufacturer;
		j["tags"] = tags;
		j["parameter_program"] = parameter_program.get_code();
		j["parameter_set"] = parameter_set_serialize(parameter_set);
		if(alternate_for && alternate_for->uuid != uuid)
			j["alternate_for"] = (std::string)alternate_for->uuid;
		j["model_filename"] = model_filename;
		j["junctions"] = json::object();
		for(const auto &it: junctions) {
			j["junctions"][(std::string)it.first] = it.second.serialize();
		}
		j["lines"] = json::object();
		for(const auto &it: lines) {
			j["lines"][(std::string)it.first] = it.second.serialize();
		}
		j["arcs"] = json::object();
		for(const auto &it: arcs) {
			j["arcs"][(std::string)it.first] = it.second.serialize();
		}
		j["texts"] = json::object();
		for(const auto &it: texts) {
			j["texts"][(std::string)it.first] = it.second.serialize();
		}
		j["pads"] = json::object();
		for(const auto &it: pads) {
			j["pads"][(std::string)it.first] = it.second.serialize();
		}
		j["polygons"] = json::object();
		for(const auto &it: polygons) {
			j["polygons"][(std::string)it.first] = it.second.serialize();
		}
		return j;
	}

	UUID Package::get_uuid() const {
		return uuid;
	}

}
