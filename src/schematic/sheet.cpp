#include "schematic/sheet.hpp"
#include "pool/part.hpp"
#include "pool/pool.hpp"
#include "util/util.hpp"
#include "logger/logger.hpp"
#include "logger/log_util.hpp"
#include "common/object_descr.hpp"

namespace horizon {

	Sheet::Sheet(const UUID &uu, const json &j, Block &block, Pool &pool):
			uuid(uu),
			name(j.at("name").get<std::string>()),
			index(j.at("index").get<unsigned int>())
		{
			Logger::log_info("loading sheet "+name, Logger::Domain::SCHEMATIC);
			if(j.count("frame")) {
				frame = Frame(j.at("frame"));
			}
			{
				const json &o = j["junctions"];
				for (auto it = o.cbegin(); it != o.cend(); ++it) {
					auto u = UUID(it.key());
					load_and_log(junctions, ObjectType::JUNCTION, std::forward_as_tuple(u, it.value()), Logger::Domain::SCHEMATIC);
				}
			}
			{
				const json &o = j["symbols"];
				for (auto it = o.cbegin(); it != o.cend(); ++it) {
					auto u = UUID(it.key());
					load_and_log(symbols, ObjectType::SCHEMATIC_SYMBOL, std::forward_as_tuple(u, it.value(), pool, &block), Logger::Domain::SCHEMATIC);
				}
			}
			{
				const json &o = j["texts"];
				for (auto it = o.cbegin(); it != o.cend(); ++it) {
					auto u = UUID(it.key());
					load_and_log(texts, ObjectType::TEXT, std::forward_as_tuple(u, it.value()), Logger::Domain::SCHEMATIC);
				}
			}
			{
				const json &o = j["net_labels"];
				for (auto it = o.cbegin(); it != o.cend(); ++it) {
					auto u = UUID(it.key());
					load_and_log(net_labels, ObjectType::NET_LABEL, std::forward_as_tuple(u, it.value(), this), Logger::Domain::SCHEMATIC);
				}
			}
			{
				const json &o = j["power_symbols"];
				for (auto it = o.cbegin(); it != o.cend(); ++it) {
					auto u = UUID(it.key());
					load_and_log(power_symbols, ObjectType::POWER_SYMBOL, std::forward_as_tuple(u, it.value(), this, &block), Logger::Domain::SCHEMATIC);
				}
			}
			{
				const json &o = j["bus_rippers"];
				for (auto it = o.cbegin(); it != o.cend(); ++it) {
					auto u = UUID(it.key());
					load_and_log(bus_rippers, ObjectType::BUS_RIPPER, std::forward_as_tuple(u, it.value(), *this, block), Logger::Domain::SCHEMATIC);
				}
			}
			{
				const json &o = j["net_lines"];
				for (auto it = o.cbegin(); it != o.cend(); ++it) {
					auto u = UUID(it.key());
					load_and_log(net_lines, ObjectType::LINE_NET, std::forward_as_tuple(u, it.value(), this), Logger::Domain::SCHEMATIC);
				}
			}
			{
				const json &o = j["bus_labels"];
				for (auto it = o.cbegin(); it != o.cend(); ++it) {
					auto u = UUID(it.key());
					bus_labels.emplace(std::make_pair(u, BusLabel(u, it.value(), *this, block)));
				}
			}
		}

	Sheet::Sheet(const UUID &uu): uuid(uu), name("First sheet"), index(1)  {}

	LineNet *Sheet::split_line_net(LineNet *it, Junction *ju) {
		auto uu = UUID::random();
		LineNet *li = &net_lines.emplace(std::make_pair(uu, uu)).first->second;
		li->from.connect(ju);
		li->to = it->to;
		li->net = it->net;
		li->bus = it->bus;
		li->net_segment = it->net_segment;

		it->to.connect(ju);
		return li;
	}

	void Sheet::merge_net_lines(LineNet *a, LineNet *b, Junction *ju) {
		if(a->from.junc==ju) {
			if(b->from.junc==ju) {
				a->from = b->to;
			}
			else {
				a->from = b->from;
			}
		}
		else {
			assert(a->to.junc==ju);
			if(b->from.junc==ju) {
				a->to = b->to;
			}
			else {
				a->to = b->from;
			}
		}

		//delete junction
		//delete b
		junctions.erase(ju->uuid);
		net_lines.erase(b->uuid);
		if(a->from.is_junc() && a->to.is_junc() &&  a->from.junc == a->to.junc) {
			net_lines.erase(a->uuid);
		}
	}

	void Sheet::expand_symbols(void) {
		for(auto &it_sym: symbols) {
			SchematicSymbol &schsym = it_sym.second;
			if(schsym.symbol.unit->uuid != schsym.gate->unit->uuid) {
				throw std::logic_error("unit mismatch");
			}
			schsym.symbol = *schsym.pool_symbol;
			schsym.symbol.expand();
			if(!schsym.display_directions) {
				for(auto &it_pin: schsym.symbol.pins) {
					it_pin.second.direction = Pin::Direction::PASSIVE;
				}
			}
			schsym.symbol.apply_placement(schsym.placement);

			if(schsym.pin_display_mode == SchematicSymbol::PinDisplayMode::ALL) {
				for(auto &it_pin: schsym.symbol.pins) {
					auto pin_uuid = it_pin.first;
					it_pin.second.name = "";
					for(auto &pin_name: schsym.gate->unit->pins.at(pin_uuid).names) {
						it_pin.second.name += pin_name + " ";
					}
					it_pin.second.name += "(" + schsym.gate->unit->pins.at(pin_uuid).primary_name + ")";
				}
			}
			else {
				for(const auto &it_name_index: schsym.component->pin_names) {
					if(it_name_index.first.at(0) == schsym.gate->uuid) {
						if(it_name_index.second != -1) {
							auto pin_uuid = it_name_index.first.at(1);
							if(schsym.pin_display_mode == SchematicSymbol::PinDisplayMode::SELECTED_ONLY)
								schsym.symbol.pins.at(pin_uuid).name =  schsym.gate->unit->pins.at(pin_uuid).names.at(it_name_index.second);
							else if(schsym.pin_display_mode == SchematicSymbol::PinDisplayMode::BOTH)
								schsym.symbol.pins.at(pin_uuid).name =  schsym.gate->unit->pins.at(pin_uuid).names.at(it_name_index.second) + " (" + schsym.gate->unit->pins.at(pin_uuid).primary_name + ")";
						}
					}
				}
			}
			if(schsym.component->part) {
				for(auto &it_pin: schsym.symbol.pins) {
					it_pin.second.pad = "";
				}
				std::map<SymbolPin*, std::vector<std::string>> pads;
				for(const auto &it_pad_map: schsym.component->part->pad_map) {
					if(it_pad_map.second.gate == schsym.gate) {
						if(schsym.symbol.pins.count(it_pad_map.second.pin->uuid)) {
							 pads[&schsym.symbol.pins.at(it_pad_map.second.pin->uuid)].push_back(schsym.component->part->package->pads.at(it_pad_map.first).name);
						}
					}
				}
				for(auto &it_pin: pads) {
					std::sort(it_pin.second.begin(), it_pin.second.end(),  [](const auto &a, const auto &b){return strcmp_natural(a,b)<0;});
					for(const auto &pad: it_pin.second) {
						it_pin.first->pad += pad + " ";
					}
				}
			}
			for(auto &it_text: schsym.symbol.texts) {
				it_text.second.text = schsym.replace_text(it_text.second.text);
			}

			for(auto &it_text: schsym.texts) {
				it_text->text_override = schsym.replace_text(it_text->text, &it_text->overridden);
			}
		}
		for(auto &it_line: net_lines) {
			LineNet &line = it_line.second;
			line.update_refs(*this);
		}
	}

	void Sheet::vacuum_junctions() {
		for(auto it = junctions.begin(); it != junctions.end();) {
			if(it->second.connection_count == 0 && !it->second.has_via) {
				it = junctions.erase(it);
			}
			else {
				it++;
			}
		}
	}

	void Sheet::simplify_net_lines(bool simplify) {
		unsigned int n_merged = 1;
		while(n_merged) {
			n_merged = 0;
			for(auto &it_junc: junctions) {
				if(!simplify) {
					it_junc.second.net = nullptr;
					it_junc.second.has_via = false;
				}
				it_junc.second.connection_count = 0;
			}
			for(auto &it_rip: bus_rippers) {
				it_rip.second.connection_count = 0;
			}
			std::set<UUID> junctions_with_label;
			for(const auto &it: net_labels) {
				junctions_with_label.emplace(it.second.junction->uuid);
				it.second.junction->has_via = true;
			}
			for(const auto &it: bus_labels) {
				junctions_with_label.emplace(it.second.junction->uuid);
				it.second.junction->has_via = true;
			}
			for(const auto &it: bus_rippers) {
				junctions_with_label.emplace(it.second.junction->uuid);
				it.second.junction->has_via = true;
			}
			for(const auto &it: power_symbols) {
				junctions_with_label.emplace(it.second.junction->uuid);
				it.second.junction->connection_count++;
			}

			std::map<UUID,std::set<UUID>> junction_connections;
			for(auto &it_line: net_lines) {
				if(!simplify) {
					it_line.second.net = nullptr;
				}
				for(auto &it_ft: {it_line.second.from, it_line.second.to}) {
					if(it_ft.is_junc()) {
						it_ft.junc->connection_count++;
						if(!junction_connections.count(it_ft.junc.uuid)) {
							junction_connections.emplace(it_ft.junc.uuid, std::set<UUID>());
						}
						junction_connections.at(it_ft.junc.uuid).emplace(it_line.first);
					}
					else if(it_ft.is_bus_ripper()) {
						it_ft.bus_ripper->connection_count++;
					}
				}
			}
			if(simplify) {
				for(const auto &it: junction_connections) {
					if(it.second.size() == 2) {
						const auto &connections = it.second;
						auto itc = connections.begin();
						if(net_lines.count(*itc) == 0)
							continue;
						auto &line_a = net_lines.at(*itc);
						itc++;
						if(net_lines.count(*itc) == 0)
							continue;
						auto &line_b = net_lines.at(*itc);

						auto va = line_a.to.get_position()-line_a.from.get_position();
						auto vb = line_b.to.get_position()-line_b.from.get_position();

						if((va.dot(vb))*(va.dot(vb)) == va.mag_sq()*vb.mag_sq()) {
							if(junctions_with_label.count(it.first) == 0) {
								merge_net_lines(&line_a, &line_b, &junctions.at(it.first));
								n_merged++;
							}
						}
					}
				}
			}
		}
	}

	void Sheet::fix_junctions() {
		for(auto &it_junc: junctions) {
			auto ju = &it_junc.second;
			for(auto &it_li: net_lines) {
				auto li = &it_li.second;
				if(ju->net_segment == li->net_segment && li->from.junc != ju && li->to.junc != ju) {
					if(li->coord_on_line(ju->position)) {
						split_line_net(li, ju);
						ju->connection_count += 2;
					}
				}
			}
		}
	}

	void Sheet::delete_duplicate_net_lines() {
		std::set<std::pair<LineNet::Connection,LineNet::Connection>> conns;
		map_erase_if(net_lines, [&conns](auto &li) {
			bool del = false;
			if(conns.emplace(li.second.from, li.second.to).second == false)
				del = true;
			if(conns.emplace(li.second.to, li.second.from).second == false)
				del = true;
			if(del) {
				for(const auto &it_ft: {li.second.from, li.second.to}) {
					if(it_ft.is_junc() && it_ft.junc->connection_count)
						it_ft.junc->connection_count--;
				}
			}
			return del;
		});
	}

	void Sheet::delete_dependants() {
		auto label_it = net_labels.begin();
		while(label_it != net_labels.end()) {
			if(junctions.count(label_it->second.junction.uuid) == 0) {
				label_it = net_labels.erase(label_it);
			}
			else {
				label_it++;
			}
		}
		auto bus_label_it = bus_labels.begin();
		while(bus_label_it != bus_labels.end()) {
			if(junctions.count(bus_label_it->second.junction.uuid) == 0) {
				bus_label_it = bus_labels.erase(bus_label_it);
			}
			else {
				bus_label_it++;
			}
		}
		auto ps_it = power_symbols.begin();
		while(ps_it != power_symbols.end()) {
			if(junctions.count(ps_it->second.junction.uuid) == 0) {
				ps_it = power_symbols.erase(ps_it);
			}
			else {
				ps_it++;
			}
		}
	}

	void Sheet::propagate_net_segments() {
		for(auto &it: junctions) {
			it.second.net_segment = UUID();
		}
		for(auto &it: bus_rippers) {
			it.second.net_segment = UUID();
		}
		for(auto &it: net_lines) {
			it.second.net_segment = UUID();
		}
		for(auto &it: symbols) {
			for(auto &it_pin: it.second.symbol.pins) {
				it_pin.second.net_segment = UUID();
				it_pin.second.connected_net_lines.clear();
			}
		}
		unsigned int run = 1;
		while(run) {
			run = 0;
			for(auto &it: net_lines) {
				if(!it.second.net_segment) {
					it.second.net_segment = UUID::random();
					run = 1;
					break;
				}
			}
			if(run == 0) {
				break;
			}
			unsigned int n_assigned = 1;
			while(n_assigned) {
				n_assigned = 0;
				for(auto &it: net_lines) {
					if(it.second.net_segment) { //net line with uuid
						for(auto &it_ft: {it.second.from, it.second.to}) {
							if(it_ft.is_junc() && !it_ft.junc->net_segment) {
								it_ft.junc->net_segment = it.second.net_segment;
								n_assigned++;
							}
							else if(it_ft.is_pin() && !it_ft.pin->net_segment) {
								it_ft.pin->net_segment = it.second.net_segment;
								it_ft.pin->connected_net_lines.emplace(it.first, &it.second);
								n_assigned++;
							}
							else if(it_ft.is_bus_ripper() && !it_ft.bus_ripper->net_segment) {
								it_ft.bus_ripper->net_segment = it.second.net_segment;
								n_assigned++;
							}
						}
					}
					else { //net line without uuid
						for(auto &it_ft: {it.second.from, it.second.to}) {
							if(it_ft.is_junc() && it_ft.junc->net_segment) {
								it.second.net_segment = it_ft.junc->net_segment;
								n_assigned++;
							}
							else if(it_ft.is_pin() && it_ft.pin->net_segment) {
								it.second.net_segment = it_ft.pin->net_segment;
								it_ft.pin->connected_net_lines.emplace(it.first, &it.second);
								n_assigned++;
							}
							else if(it_ft.is_bus_ripper() && it_ft.bus_ripper->net_segment) {
								it.second.net_segment = it_ft.bus_ripper->net_segment;
								n_assigned++;
							}
						}
					}
				}
			}
		}
		//fix single junctions
		for(auto &it: junctions) {
			if(!it.second.net_segment) {
				it.second.net_segment = UUID::random();
			}
		}
	}

	NetSegmentInfo::NetSegmentInfo(Junction *ju): position(ju->position), net(ju->net), bus(ju->bus) {}
	NetSegmentInfo::NetSegmentInfo(LineNet *li): position((li->to.get_position()+li->from.get_position())/2), net(li->net), bus(li->bus) {}
	bool NetSegmentInfo::is_bus() const {
		if(bus) {
			assert(!net);
			return true;
		}
		return false;
	}

	std::map<UUID, NetSegmentInfo> Sheet::analyze_net_segments(bool place_warnings) {
		std::map<UUID, NetSegmentInfo> net_segments;
		for(auto &it: net_lines) {
			auto ns = it.second.net_segment;
			net_segments.emplace(ns, NetSegmentInfo(&it.second));
		}
		for(auto &it: junctions) {
			auto ns = it.second.net_segment;
			net_segments.emplace(ns, NetSegmentInfo(&it.second));
		}
		for(const auto &it: net_labels) {
			if(net_segments.count(it.second.junction->net_segment)) {
				net_segments.at(it.second.junction->net_segment).has_label = true;
			}
		}
		for(const auto &it: power_symbols) {
			if(net_segments.count(it.second.junction->net_segment)) {
				net_segments.at(it.second.junction->net_segment).has_power_sym = true;
				net_segments.at(it.second.junction->net_segment).has_label = true;
			}
		}
		for(const auto &it: bus_rippers) {
			if(net_segments.count(it.second.net_segment)) {
				net_segments.at(it.second.net_segment).has_label = true;
			}
		}

		if(!place_warnings)
			return net_segments;

		for(const auto &it: net_segments) {
			if(!it.second.has_label) {
				//std::cout<< "ns no label" << (std::string)it.second.net << std::endl;
				if(it.second.net && it.second.net->is_named()) {
					warnings.emplace_back(it.second.position, "Label missing");
				}
			}
			if(!it.second.has_power_sym) {
				//std::cout<< "ns no label" << (std::string)it.second.net << std::endl;
				if(it.second.net && it.second.net->is_power) {
					warnings.emplace_back(it.second.position, "Power sym missing");
				}
			}
		}
		std::set<Net*> nets_unnamed, ambigous_nets;

		for(const auto &it: net_segments) {
			if(it.second.net && !it.second.net->is_named()) {
				auto x= nets_unnamed.emplace(it.second.net);
				if(x.second == false) { //already exists
					ambigous_nets.emplace(it.second.net);
				}
			}
		}
		for(const auto &it: net_segments) {
			if(ambigous_nets.count(it.second.net)) {
				if(!it.second.has_label)
					warnings.emplace_back(it.second.position, "Ambigous nets");
			}
		}
		for(const auto &it: net_lines) {
			if(it.second.from.get_position() == it.second.to.get_position()) {
				warnings.emplace_back(it.second.from.get_position(), "Zero length line");
			}
		}
		return net_segments;
	}

	std::set<UUIDPath<3>> Sheet::get_pins_connected_to_net_segment(const UUID &uu_segment) {
		std::set<UUIDPath<3>> r;
		for(const auto &it_sym: symbols) {
			for(const auto &it_pin: it_sym.second.symbol.pins) {
				if(it_pin.second.net_segment == uu_segment) {
					r.emplace(it_sym.second.component->uuid, it_sym.second.gate->uuid, it_pin.first);
				}
			}
		}
		return r;
	}

	void Sheet::replace_junction(Junction *j, SchematicSymbol *sym, SymbolPin *pin) {
		for(auto &it_line: net_lines) {
			for(auto it_ft: {&it_line.second.from, &it_line.second.to}) {
				if(it_ft->junc == j) {
					it_ft->connect(sym, pin);
				}
			}
		}
	}

	void Sheet::merge_junction(Junction *j, Junction *into) {
		for(auto &it: net_lines) {
			if(it.second.from.junc == j) {
				it.second.from.junc = into;
			}
			if(it.second.to.junc == j) {
				it.second.to.junc = into;
			}
		}
		for(auto &it: net_labels) {
			if(it.second.junction == j)
				it.second.junction = into;
		}
		for(auto &it: bus_labels) {
			if(it.second.junction == j)
				it.second.junction = into;
		}
		for(auto &it:bus_rippers) {
			if(it.second.junction == j)
				it.second.junction = into;
		}
		for(auto &it: power_symbols) {
			if(it.second.junction == j)
				it.second.junction = into;
		}

		junctions.erase(j->uuid);
	}

	Junction *Sheet::replace_bus_ripper(BusRipper *rip) {
		auto uu = UUID::random();
		Junction *j = &junctions.emplace(uu, uu).first->second;
		j->net = rip->bus_member->net;
		j->position = rip->get_connector_pos();

		for(auto &it_line: net_lines) {
			for(auto it_ft: {&it_line.second.from, &it_line.second.to}) {
				if(it_ft->bus_ripper == rip) {
					it_ft->connect(j);
				}
			}
		}
		return j;

	}

	/*void Sheet::connect(SchematicSymbol *sym, SymbolPin *pin, PowerSymbol *power_sym) {
		auto uu = UUID::random();

		//connect pin
		sym->component->connections.emplace(UUIDPath<2>(sym->gate->uuid, pin->uuid), power_sym->net);

		uu = UUID::random();
		auto *line = &net_lines.emplace(uu, uu).first->second;
		line->net = power_sym->net;
		line->from.connect(sym, pin);
		line->to.connect(power_sym);
	}*/

	json Sheet::serialize() const {
			json j;
			j["name"] = name;
			j["index"] = index;
			j["frame"] = frame.serialize();
			j["symbols"] = json::object();

			for(const auto &it : symbols) {
				j["symbols"][(std::string)it.first] = it.second.serialize();
			}

			j["junctions"] = json::object();
			for(const auto &it : junctions) {
				j["junctions"][(std::string)it.first] = it.second.serialize();
			}
			j["net_lines"] = json::object();
			for(const auto &it : net_lines) {
				j["net_lines"][(std::string)it.first] = it.second.serialize();
			}
			j["texts"] = json::object();
			for(const auto &it : texts) {
				j["texts"][(std::string)it.first] = it.second.serialize();
			}
			j["net_labels"] = json::object();
			for(const auto &it : net_labels) {
				j["net_labels"][(std::string)it.first] = it.second.serialize();
			}
			j["power_symbols"] = json::object();
			for(const auto &it : power_symbols) {
				j["power_symbols"][(std::string)it.first] = it.second.serialize();
			}
			j["bus_labels"] = json::object();
			for(const auto &it : bus_labels) {
				j["bus_labels"][(std::string)it.first] = it.second.serialize();
			}
			j["bus_rippers"] = json::object();
			for(const auto &it : bus_rippers) {
				j["bus_rippers"][(std::string)it.first] = it.second.serialize();
			}

			return j;
		}
}
