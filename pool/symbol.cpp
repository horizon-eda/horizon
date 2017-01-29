#include "symbol.hpp"
#include "junction.hpp"
#include "line.hpp"
#include "lut.hpp"
#include "json.hpp"
#include <iostream>
#include <algorithm>

namespace horizon {
	
	static const LutEnumStr<Orientation> orientation_lut = {
		{"up", 		Orientation::UP},
		{"down", 	Orientation::DOWN},
		{"left", 	Orientation::LEFT},
		{"right",	Orientation::RIGHT},
	};
	
	SymbolPin::SymbolPin(const UUID &uu, const json &j):
		uuid(uu),
		position(j["position"].get<std::vector<int64_t>>()),
		length(j["length"]),
		name_visible(j.value("name_visible", true)),
		pad_visible(j.value("pad_visible", true)),
		orientation(orientation_lut.lookup(j["orientation"]))
	{
	}
	
	SymbolPin::SymbolPin(UUID uu): uuid(uu), name_visible(true), pad_visible(true) {}
	
	json SymbolPin::serialize() const {
		json j;
		j["position"] = position.as_array();
		j["length"] = length;
		j["orientation"] = orientation_lut.lookup_reverse(orientation);
		j["name_visible"] = name_visible;
		j["pad_visible"] = pad_visible;
		return j;
	}

	UUID SymbolPin::get_uuid() const {
		return uuid;
	}

	Symbol::Symbol(const UUID &uu, const json &j, Object &obj):
		uuid(uu),
		unit(obj.get_unit(j.at("unit").get<std::string>())),
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
	}
	
	Symbol::Symbol(const UUID &uu): uuid(uu) {}

	Symbol Symbol::new_from_file(const std::string &filename, Object &obj) {
				json j;
				std::ifstream ifs(filename);
				if(!ifs.is_open()) {
					throw std::runtime_error("file "  +filename+ " not opened");
				}
				ifs>>j;
				ifs.close();
				return Symbol(UUID(j["uuid"].get<std::string>()), j, obj);
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
		texts(sym.texts)
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
				a = Coordi::min(a, it.second.position);
				b = Coordi::max(b, it.second.position);
			}
		}
		return std::make_pair(a,b);
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
		j["texts"] = json::object();
		for(const auto &it: texts) {
			j["texts"][(std::string)it.first] = it.second.serialize();
		}
		return j;
	}
}
