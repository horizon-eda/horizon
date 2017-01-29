#include "package.hpp"
#include "json.hpp"
#include "pool.hpp"

namespace horizon {

	Package::Package(const UUID &uu, const json &j, Pool &pool):
			uuid(uu),
			name(j.at("name").get<std::string>())

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
		if(j.count("tags")) {
			tags = j.at("tags").get<std::set<std::string>>();
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

	Package::Package(const UUID &uu): uuid(uu) {}

	Junction *Package::get_junction(const UUID &uu) {
		return &junctions.at(uu);
	}

	Package::Package(const Package &pkg):
		uuid(pkg.uuid),
		name(pkg.name),
		tags(pkg.tags),
		junctions(pkg.junctions),
		lines(pkg.lines),
		arcs(pkg.arcs),
		texts(pkg.texts),
		pads(pkg.pads),
		polygons(pkg.polygons),
		warnings(pkg.warnings)
	{
		update_refs();
	}

	void Package::operator=(Package const &pkg) {
		uuid = pkg.uuid;
		name = pkg.name;
		tags = pkg.tags;
		junctions = pkg.junctions;
		lines = pkg.lines;
		arcs = pkg.arcs;
		texts = pkg.texts;
		pads = pkg.pads;
		polygons = pkg.polygons;
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
	}

	std::pair<Coordi, Coordi> Package::get_bbox() const {
		Coordi a;
		Coordi b;
		for(const auto &it: pads) {
			auto bb_pad = it.second.padstack.get_bbox();
			bb_pad.first = it.second.placement.transform(bb_pad.first);
			bb_pad.second = it.second.placement.transform(bb_pad.second);
			a = Coordi::min(a, bb_pad.first);
			a = Coordi::min(a, bb_pad.second);
			b = Coordi::max(b, bb_pad.first);
			b = Coordi::max(b, bb_pad.second);
		}
		return std::make_pair(a,b);
	}


	json Package::serialize() const {
		json j;
		j["uuid"] = (std::string)uuid;
		j["type"] = "package";
		j["name"] = name;
		j["tags"] = tags;

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

}
