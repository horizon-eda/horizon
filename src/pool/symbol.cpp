#include "pool/symbol.hpp"
#include "common/junction.hpp"
#include "common/line.hpp"
#include "common/lut.hpp"
#include "pool/pool.hpp"
#include "util/util.hpp"
#include <iostream>
#include <algorithm>

namespace horizon {

	static const LutEnumStr<SymbolPin::Decoration::Driver> decoration_driver_lut = {
		{"default",	 				SymbolPin::Decoration::Driver::DEFAULT},
		{"open_collector",			SymbolPin::Decoration::Driver::OPEN_COLLECTOR},
		{"open_collector_pullup",	SymbolPin::Decoration::Driver::OPEN_COLLECTOR_PULLUP},
		{"open_emitter",			SymbolPin::Decoration::Driver::OPEN_EMITTER},
		{"open_emitter_pulldown",	SymbolPin::Decoration::Driver::OPEN_EMITTER_PULLDOWN},
		{"tristate",				SymbolPin::Decoration::Driver::TRISTATE},
	};

	SymbolPin::Decoration::Decoration() {}
	SymbolPin::Decoration::Decoration(const json &j):
		dot(j.at("dot")),
		clock(j.at("clock")),
		schmitt(j.at("schmitt")),
		driver(decoration_driver_lut.lookup(j.at("driver")))
	{}

	json SymbolPin::Decoration::serialize() const {
		json j;
		j["dot"] = dot;
		j["clock"] = clock;
		j["schmitt"] = schmitt;
		j["driver"] = decoration_driver_lut.lookup_reverse(driver);
		return j;
	}

	
	SymbolPin::SymbolPin(const UUID &uu, const json &j):
		uuid(uu),
		position(j["position"].get<std::vector<int64_t>>()),
		length(j["length"]),
		name_visible(j.value("name_visible", true)),
		pad_visible(j.value("pad_visible", true)),
		orientation(orientation_lut.lookup(j["orientation"]))
	{
		if(j.count("decoration")) {
			decoration = Decoration(j.at("decoration"));
		}
	}
	
	SymbolPin::SymbolPin(UUID uu): uuid(uu), name_visible(true), pad_visible(true) {}
	
	static const std::map<Orientation, Orientation> omap_90 = {
		{Orientation::LEFT, Orientation::DOWN},
		{Orientation::UP, Orientation::LEFT},
		{Orientation::RIGHT, Orientation::UP},
		{Orientation::DOWN, Orientation::RIGHT},
	};
	static const std::map<Orientation, Orientation> omap_180 = {
		{Orientation::LEFT, Orientation::RIGHT},
		{Orientation::UP, Orientation::DOWN},
		{Orientation::RIGHT, Orientation::LEFT},
		{Orientation::DOWN, Orientation::UP},
	};
	static const std::map<Orientation, Orientation> omap_270 = {
		{Orientation::LEFT, Orientation::UP},
		{Orientation::UP, Orientation::RIGHT},
		{Orientation::RIGHT, Orientation::DOWN},
		{Orientation::DOWN, Orientation::LEFT},
	};
	static const std::map<Orientation, Orientation> omap_mirror = {
		{Orientation::LEFT, Orientation::RIGHT},
		{Orientation::UP, Orientation::UP},
		{Orientation::RIGHT, Orientation::LEFT},
		{Orientation::DOWN, Orientation::DOWN},
	};

	Orientation SymbolPin::get_orientation_for_placement(const Placement &pl) const {
		Orientation pin_orientation = orientation;
		auto angle = pl.get_angle();
		if(angle == 16384) {
			pin_orientation = omap_90.at(pin_orientation);
		}
		if(angle == 32768) {
			pin_orientation = omap_180.at(pin_orientation);
		}
		if(angle == 49152) {
			pin_orientation = omap_270.at(pin_orientation);
		}
		if(pl.mirror) {
			pin_orientation = omap_mirror.at(pin_orientation);
		}
		return pin_orientation;
	}

	json SymbolPin::serialize() const {
		json j;
		j["position"] = position.as_array();
		j["length"] = length;
		j["orientation"] = orientation_lut.lookup_reverse(orientation);
		j["name_visible"] = name_visible;
		j["pad_visible"] = pad_visible;
		j["decoration"] = decoration.serialize();
		return j;
	}

	UUID SymbolPin::get_uuid() const {
		return uuid;
	}

	Symbol::Symbol(const UUID &uu, const json &j, Pool &pool):
		uuid(uu),
		unit(pool.get_unit(j.at("unit").get<std::string>())),
		name(j.value("name", ""))
	{
		if(j.count("junctions")) {
			const json &o = j["junctions"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID(it.key());
				junctions.emplace(std::make_pair(u, Junction(u, it.value())));
			}
		}
		if(j.count("lines")) {
			const json &o = j["lines"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID(it.key());
				lines.emplace(std::make_pair(u, Line(u, it.value(), *this)));
			}
		}
		if(j.count("pins")) {
			const json &o = j["pins"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID(it.key());
				pins.emplace(std::make_pair(u, SymbolPin(u, it.value())));
			}
		}
		if(j.count("arcs")) {
			const json &o = j["arcs"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID(it.key());
				arcs.emplace(std::make_pair(u, Arc(u, it.value(), *this)));
			}
		}
		if(j.count("texts")) {
			const json &o = j["texts"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID(it.key());
				texts.emplace(std::make_pair(u, Text(u, it.value())));
			}
		}
		if(j.count("polygons")) {
			const json &o = j["polygons"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID(it.key());
				polygons.emplace(std::piecewise_construct, std::forward_as_tuple(u), std::forward_as_tuple(u, it.value()));
			}
		}
		map_erase_if(polygons, [](const auto &a){return a.second.vertices.size()==0;});
		if(j.count("text_placements")) {
			const json &o = j["text_placements"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				std::string view_str(it.key());
				int angle = std::stoi(view_str);
				int mirror = view_str.find("m")!=std::string::npos;
				const json &k = it.value();
				for (auto it2 = k.cbegin(); it2 != k.cend(); ++it2) {
					UUID u(it2.key());
					if(texts.count(u)) {
						Placement placement (it2.value());
						std::tuple<int, bool, UUID> key (angle ,mirror, u);
						text_placements[key] = placement;
					}
				}
			}
		}
	}
	
	Symbol::Symbol(const UUID &uu): uuid(uu) {}

	Symbol Symbol::new_from_file(const std::string &filename, Pool &pool) {
				json j;
				std::ifstream ifs(filename);
				if(!ifs.is_open()) {
					throw std::runtime_error("file "  +filename+ " not opened");
				}
				ifs>>j;
				ifs.close();
				return Symbol(UUID(j["uuid"].get<std::string>()), j, pool);
			}

	Junction *Symbol::get_junction(const UUID &uu)  {
		return junctions.count(uu)?&junctions.at(uu):nullptr;
	}
	SymbolPin *Symbol::get_symbol_pin(const UUID &uu)  {
		return pins.count(uu)?&pins.at(uu):nullptr;
	}
	
	void Symbol::update_refs() {
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
	
	Symbol::Symbol(const Symbol &sym):
		uuid(sym.uuid),
		unit(sym.unit),
		name(sym.name),
		pins(sym.pins),
		junctions(sym.junctions),
		lines(sym.lines),
		arcs(sym.arcs),
		texts(sym.texts),
		polygons(sym.polygons),
		text_placements(sym.text_placements)
	{
		update_refs();
	}
	
	void Symbol::operator=(Symbol const &sym) {
		uuid = sym.uuid;
		unit = sym.unit;
		name = sym.name;
		pins = sym.pins;
		junctions = sym.junctions;
		lines = sym.lines;
		arcs = sym.arcs;
		texts = sym.texts;
		polygons = sym.polygons;
		text_placements = sym.text_placements;
		update_refs();
	}
	
	void Symbol::expand() {
		std::vector<UUID> keys;
		keys.reserve(pins.size());
		for(const auto &it: pins) {
			keys.push_back(it.first);
		}
		for(const auto &uu: keys) {
			if(unit->pins.count(uu)) {
				SymbolPin &p = pins.at(uu);
				p.pad = "$PAD";
				p.name = unit->pins.at(uu).primary_name;
				p.direction = unit->pins.at(uu).direction;
			}
			else {
				pins.erase(uu);
			}
		}
	}

	std::pair<Coordi, Coordi> Symbol::get_bbox(bool all) const {
		Coordi a;
		Coordi b;
		for(const auto &it: junctions) {
			a = Coordi::min(a, it.second.position);
			b = Coordi::max(b, it.second.position);
		}
		for(const auto &it: pins) {
			a = Coordi::min(a, it.second.position);
			b = Coordi::max(b, it.second.position);
		}
		if(all) {
			for(const auto &it: texts) {
				a = Coordi::min(a, it.second.placement.shift);
				b = Coordi::max(b, it.second.placement.shift);
			}
		}
		return std::make_pair(a,b);
	}

	void Symbol::apply_placement(const Placement &p) {
		for(auto &it: texts) {
			std::tuple<int, bool, UUID> key (p.get_angle_deg() , p.mirror, it.second.uuid);
			if(text_placements.count(key)) {
				it.second.placement = text_placements.at(key);
			}
		}
	}

	json Symbol::serialize() const {
		json j;
		j["type"] = "symbol";
		j["name"] = name;
		j["uuid"] = (std::string)uuid;
		j["unit"] = (std::string)unit->uuid;
		j["junctions"] = json::object();
		for(const auto &it: junctions) {
			j["junctions"][(std::string)it.first] = it.second.serialize();
		}
		j["pins"] = json::object();
		for(const auto &it: pins) {
			j["pins"][(std::string)it.first] = it.second.serialize();
		}
		j["lines"] = json::object();
		for(const auto &it: lines) {
			j["lines"][(std::string)it.first] = it.second.serialize();
		}
		j["arcs"] = json::object();
		for(const auto &it: arcs) {
			j["arcs"][(std::string)it.first] = it.second.serialize();
		}
		j["polygons"] = json::object();
		for(const auto &it: polygons) {
			j["polygons"][(std::string)it.first] = it.second.serialize();
		}
		j["texts"] = json::object();
		for(const auto &it: texts) {
			j["texts"][(std::string)it.first] = it.second.serialize();
		}
		j["text_placements"] = json::object();
		for(const auto &it: text_placements) {
			int angle;
			bool mirror;
			UUID uu;
			std::tie(angle, mirror, uu) = it.first;
			std::string k = std::to_string(angle);
			if(mirror) {
				k+="m";
			}
			else {
				k+="n";
			}
			j["text_placements"][k][(std::string)uu] = it.second.serialize();
		}
		return j;
	}
}
