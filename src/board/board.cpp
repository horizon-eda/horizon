#include "board/board.hpp"
#include "pool/part.hpp"
#include <list>
#include "util/util.hpp"
#include "board/board_layers.hpp"
#include "logger/logger.hpp"
#include "logger/log_util.hpp"

namespace horizon {

	Board::StackupLayer::StackupLayer(int l): layer(l) {}
	Board::StackupLayer::StackupLayer(int l, const json &j):
		layer(l), thickness(j.at("thickness")), substrate_thickness(j.at("substrate_thickness")) {}

	json Board::StackupLayer::serialize() const {
		json j;
		j["thickness"] = thickness;
		j["substrate_thickness"] = substrate_thickness;
		return j;
	}

	Board::Board(const UUID &uu, const json &j, Block &iblock, Pool &pool, ViaPadstackProvider &vpp):
			uuid(uu),
			block(&iblock),
			name(j.at("name").get<std::string>()),
			n_inner_layers(j.value("n_inner_layers", 0))
		{
		Logger::log_info("loading board "+(std::string)uuid, Logger::Domain::BOARD);
		if(j.count("stackup")) {
			const json &o = j["stackup"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				int l = std::stoi(it.key());
				stackup.emplace(std::piecewise_construct, std::forward_as_tuple(l), std::forward_as_tuple(l, it.value()));
			}
		}
		set_n_inner_layers(n_inner_layers);
		if(j.count("polygons")) {
			const json &o = j["polygons"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID(it.key());
				load_and_log(polygons, ObjectType::POLYGON, std::forward_as_tuple(u, it.value()), Logger::Domain::BOARD);
			}
		}
		map_erase_if(polygons, [](const auto &a){return a.second.vertices.size()==0;});
		if(j.count("holes")) {
			const json &o = j["holes"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID(it.key());
				load_and_log(holes, ObjectType::BOARD_HOLE, std::forward_as_tuple(u, it.value(), *block, pool), Logger::Domain::BOARD);
			}
		}
		if(j.count("packages")) {
			const json &o = j["packages"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				UUID comp_uuid (it.value().at("component").get<std::string>());
				auto u = UUID(it.key());
				if(block->components.count(comp_uuid) && block->components.at(comp_uuid).part != nullptr) {
					load_and_log(packages, ObjectType::BOARD_PACKAGE, std::forward_as_tuple(u, it.value(), *block, pool), Logger::Domain::BOARD);
				}
				else {
					Logger::log_warning("not loading package " + (std::string)u + " since component is gone");
				}
			}
		}
		if(j.count("junctions")) {
			const json &o = j["junctions"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID(it.key());
				load_and_log(junctions, ObjectType::JUNCTION, std::forward_as_tuple(u, it.value()), Logger::Domain::BOARD);
			}
		}
		if(j.count("tracks")) {
			const json &o = j["tracks"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID(it.key());
				bool valid = true;
				for(const auto &it_ft: {it.value().at("from"), it.value().at("to")}) {
					if(it_ft.at("pad") != nullptr) {
						UUIDPath<2> path(it_ft.at("pad").get<std::string>());
						valid = packages.count(path.at(0));
						if(valid) {
							auto &pkg = packages.at(path.at(0));
							if(pkg.component->part->pad_map.count(path.at(1))==0) {
								valid = false;
								break;
							}
						}
						else {
							break;
						}
					}
				}
				if(valid) {
					load_and_log(tracks, ObjectType::TRACK, std::forward_as_tuple(u, it.value(), *this), Logger::Domain::BOARD);
				}
				else {
					Logger::log_warning("not loading track " + (std::string)u + " since at least one pad is gone");
				}
			}
		}
		if(j.count("vias")) {
			const json &o = j["vias"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID(it.key());
				load_and_log(vias, ObjectType::VIA, std::forward_as_tuple(u, it.value(), *this, vpp), Logger::Domain::BOARD);
			}
		}
		if(j.count("texts")) {
			const json &o = j["texts"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID(it.key());
				load_and_log(texts, ObjectType::TEXT, std::forward_as_tuple(u, it.value()), Logger::Domain::BOARD);
			}
		}
		if(j.count("lines")) {
			const json &o = j["lines"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID(it.key());
				load_and_log(lines, ObjectType::LINE, std::forward_as_tuple(u, it.value(), *this), Logger::Domain::BOARD);
			}
		}
		if(j.count("planes")) {
			const json &o = j["planes"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID(it.key());
				load_and_log(planes, ObjectType::PLANE, std::forward_as_tuple(u, it.value(), *this), Logger::Domain::BOARD);
			}
		}
		if(j.count("dimensions")) {
			const json &o = j["dimensions"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID(it.key());
				load_and_log(dimensions, ObjectType::DIMENSION, std::forward_as_tuple(u, it.value()), Logger::Domain::BOARD);
			}
		}
		if(j.count("arcs")) {
			const json &o = j["arcs"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID(it.key());
				load_and_log(arcs, ObjectType::ARC, std::forward_as_tuple(u, it.value(), *this), Logger::Domain::BOARD);
			}
		}
		if(j.count("rules")) {
			try {
				rules.load_from_json(j.at("rules"));
				rules.cleanup(block);
			}
			catch (const std::exception &e) {
				Logger::log_warning("couldn't load rules", Logger::Domain::BOARD, e.what());
			}
		}
		if(j.count("fab_output_settings")) {
			try {
				fab_output_settings = FabOutputSettings(j.at("fab_output_settings"));
			}
			catch (const std::exception &e) {
				Logger::log_warning("couldn't load fab output settings", Logger::Domain::BOARD, e.what());
			}
		}
		fab_output_settings.update_for_board(this);
		update_refs(); //fill in smashed texts

	}

	Board Board::new_from_file(const std::string &filename, Block &block, Pool &pool, ViaPadstackProvider &vpp) {
		json j;
		std::ifstream ifs(filename);
		if(!ifs.is_open()) {
			throw std::runtime_error("file "  +filename+ " not opened");
		}
		ifs>>j;
		ifs.close();
		return Board(UUID(j["uuid"].get<std::string>()), j, block, pool, vpp);
	}

	Board::Board(const UUID &uu, Block &bl): uuid(uu), block(&bl) {
		rules.add_rule(RuleID::CLEARANCE_COPPER);
		rules.add_rule(RuleID::CLEARANCE_COPPER_OTHER);
		rules.add_rule(RuleID::TRACK_WIDTH);
		rules.add_rule(RuleID::PLANE);
		auto r = dynamic_cast<RuleTrackWidth*>(rules.get_rules(RuleID::TRACK_WIDTH).begin()->second);
		r->widths.emplace(std::piecewise_construct, std::forward_as_tuple(0), std::forward_as_tuple());
		//TBD: inner layers
		r->widths.emplace(std::piecewise_construct, std::forward_as_tuple(-100), std::forward_as_tuple());
	}

	Junction *Board::get_junction(const UUID &uu) {
		return &junctions.at(uu);
	}

	const std::map<int, Layer> &Board::get_layers() const {
		return layers;
	}

	Board::Board(const Board &brd):
		layers(brd.layers),
		uuid(brd.uuid),
		block(brd.block),
		name(brd.name),
		polygons(brd.polygons),
		holes(brd.holes),
		packages(brd.packages),
		junctions(brd.junctions),
		tracks(brd.tracks),
		airwires(brd.airwires),
		vias(brd.vias),
		texts(brd.texts),
		lines(brd.lines),
		arcs(brd.arcs),
		planes(brd.planes),
		dimensions(brd.dimensions),
		warnings(brd.warnings),
		rules(brd.rules),
		fab_output_settings(brd.fab_output_settings),
		stackup(brd.stackup),
		n_inner_layers(brd.n_inner_layers)
	{
		update_refs();
	}

	void Board::operator=(const Board &brd) {
		layers = brd.layers;
		uuid = brd.uuid;
		block = brd.block;
		name = brd.name;
		n_inner_layers = brd.n_inner_layers;
		polygons = brd.polygons;
		holes = brd.holes;
		packages.clear();
		packages = brd.packages;
		junctions = brd.junctions;
		tracks = brd.tracks;
		airwires = brd.airwires;
		vias = brd.vias;
		texts = brd.texts;
		lines = brd.lines;
		arcs = brd.arcs;
		planes = brd.planes;
		dimensions = brd.dimensions;
		warnings = brd.warnings;
		rules = brd.rules;
		fab_output_settings = brd.fab_output_settings;
		stackup = brd.stackup;
		update_refs();
	}

	void Board::update_refs() {
		for(auto &it: packages) {
			it.second.component.update(block->components);
			for(auto &it_pad: it.second.package.pads) {
				it_pad.second.net.update(block->nets);
			}
			for(auto &it_text: it.second.texts) {
				it_text.update(texts);
			}
		}
		for(auto &it: tracks) {
			it.second.update_refs(*this);
		}
		for(auto &it: airwires) {
			it.second.update_refs(*this);
		}
		for(auto &it: net_segments) {
			it.second.update(block->nets);
		}
		for(auto &it: vias) {
			it.second.junction.update(junctions);
			it.second.net_set.update(block->nets);
		}
		for(auto &it: holes) {
			it.second.net.update(block->nets);
		}
		for(auto &it: junctions) {
			it.second.net.update(block->nets);
		}
		for(auto &it: lines) {
			auto &line = it.second;
			line.to = &junctions.at(line.to.uuid);
			line.from = &junctions.at(line.from.uuid);
		}
		for(auto &it: arcs) {
			auto &arc = it.second;
			arc.to.update(junctions);
			arc.from.update(junctions);
			arc.center.update(junctions);
		}
		for(auto &it: planes) {
			it.second.net.update(block->nets);
			it.second.polygon.update(polygons);
			it.second.polygon->usage = &it.second;
		}
	}

	unsigned int Board::get_n_inner_layers() const {
		return n_inner_layers;
	}

	void Board::set_n_inner_layers(unsigned int n) {
		n_inner_layers = n;
		layers.clear();
		layers = {
		{100, {100, "Outline", {.6,.6, 0}}},
		{60, {60, "Top Courtyard", {.5,.5,.5}}},
		{50, {50, "Top Assembly", {.5,.5,.5}}},
		{40, {40, "Top Package", {.5,.5,.5}}},
		{30, {30, "Top Paste", {.8,.8,.8}}},
		{20, {20, "Top Silkscreen", {.9,.9,.9}}},
		{10, {10, "Top Mask", {1,.5,.5}}},
		{0, {0, "Top Copper", {1,0,0}, false, true}},
		{-100, {-100, "Bottom Copper", {0,.5,0}, true, true}},
		{-110, {-110, "Bottom Mask", {.25,.5,.25}, true}},
		{-120, {-120, "Bottom Silkscreen", {.9,.9,.9}, true}},
		{-130, {-130, "Bottom Paste", {.8,.8,.8}}},
		{-140, {-140, "Bottom Package", {.5,.5,.5}}},
		{-150, {-150, "Bottom Assembly", {.5,.5,.5}, true}},
		{-160, {-160, "Bottom Courtyard", {.5,.5,.5}}}
		};
		stackup.emplace(0, 0);
		stackup.emplace(-100, -100);
		stackup.at(-100).substrate_thickness=0;
		for(unsigned int i = 0; i<n_inner_layers; i++) {
			auto j = i+1;
			layers.emplace(std::make_pair(-j, Layer(-j, "Inner "+std::to_string(j), {1,1,0}, false, true)));
			stackup.emplace(-j, -j);
		}
		map_erase_if(stackup, [this](const auto &x){return layers.count(x.first)==0;});
	}

	static void flip_package_layer(int &layer) {
		if(layer == -1)
			return;
		layer = -layer-100;
	}


	bool Board::propagate_net_segments() {
		net_segments.clear();
		net_segments.emplace(UUID(), nullptr);

		bool run = true;
		while(run) {
			run = false;
			for(auto &it_pkg: packages) { //find one pad with net, but without net segment and assign it to a new net segment
				for(auto &it_pad: it_pkg.second.package.pads) {
					if(!it_pad.second.net_segment && it_pad.second.net) {
						it_pad.second.net_segment = UUID::random();
						net_segments.emplace(it_pad.second.net_segment, it_pad.second.net);
						run = true;
						break;
					}
				}
				if(run)
					break;
			}
			if(run == false) { //no pad needing assignment found, look at vias
				for(auto &it_via: vias) {
					auto ju = it_via.second.junction;
					if(!ju->net_segment && it_via.second.net_set) {
						ju->net_segment = UUID::random();
						net_segments.emplace(ju->net_segment, it_via.second.net_set);
						run = true;
						break;
					}
				}
			}
			if(run == false) //done with pads and vias
				break;

			unsigned int n_assigned = 1;
			while(n_assigned) {
				n_assigned = 0;
				for(auto &it: tracks) {
					if(it.second.net_segment) { //track with net seg
						for(auto &it_ft: {it.second.from, it.second.to}) {
							if(it_ft.is_junc() && !it_ft.junc->net_segment) { //propgate to junction
								it_ft.junc->net_segment = it.second.net_segment;
								n_assigned++;
							}
							else if(it_ft.is_pad() && !it_ft.pad->net_segment) { //propagate to pad
								it_ft.pad->net_segment = it.second.net_segment;
								n_assigned++;
							}
						}
					}
					else { //track without net seg, so propagate net segment to track
						for(auto &it_ft: {it.second.from, it.second.to}) {
							if(it_ft.is_junc() && it_ft.junc->net_segment) { //propagate from junction
								it.second.net_segment = it_ft.junc->net_segment;
								n_assigned++;
							}
							else if(it_ft.is_pad() && it_ft.pad->net_segment) {
								it.second.net_segment = it_ft.pad->net_segment; //propgate from pad
								//it_ft.pin->connected_net_lines.emplace(it.first, &it.second);
								n_assigned++;
							}
						}
					}
				}
			}
		}
		for(auto &it: junctions) { //assign net to junctions
			it.second.net = net_segments.at(it.second.net_segment);
		}

		bool done = true;
		for (auto it = tracks.begin(); it != tracks.end();) { //find tracks that connect different nets and delete these
			it->second.net = net_segments.at(it->second.net_segment);
			bool erased = false;
			for(auto &it_ft: {it->second.from, it->second.to}) {
				if(it_ft.is_pad() && it_ft.pad->net) {
					if(it->second.net != it_ft.pad->net) {
						done = false;
						tracks.erase(it++);
						erased = true;
						break;
					}
				}
			}
			if(!erased)
				it++;
		}
		//if a track has been deleted, we'll have to re-run the net segment assignment again
		return done;
	}


	void Board::vacuum_junctions() {
		for(auto it = junctions.begin(); it != junctions.end();) {
			if(it->second.connection_count == 0 && it->second.has_via==false) {
				it = junctions.erase(it);
			}
			else {
				it++;
			}
		}
	}

	void Board::expand(bool careful) {
		delete_dependants();
		warnings.clear();


		for(auto &it: junctions) {
			it.second.temp = false;
			it.second.layer = 10000;
			it.second.has_via = false;
			it.second.needs_via = false;
			it.second.connection_count = 0;
		}

		for(const auto &it: tracks) {
			for(const auto &it_ft: {it.second.from, it.second.to}) {
				if(it_ft.is_junc()) {
					auto ju = it_ft.junc;
					ju->connection_count ++;
					if(ju->layer == 10000) { //none assigned
						ju->layer = it.second.layer;
					}
					else if(ju->layer == 10001) {//invalid
						//nop
					}
					else if(ju->layer != it.second.layer) {
						ju->layer = 10001;
						ju->needs_via = true;
					}
				}
			}
			if(it.second.from.get_position() == it.second.to.get_position()) {
				warnings.emplace_back(it.second.from.get_position(), "Zero length track");
			}
		}

		for(const auto &it: lines) {
			it.second.from->connection_count++;
			it.second.to->connection_count++;
			for(const auto &ju: {it.second.from, it.second.to}) {
				if(ju->layer == 10000) { //none assigned
					ju->layer = it.second.layer;
				}
			}
		}
		for(const auto &it: arcs) {
			it.second.from->connection_count++;
			it.second.to->connection_count++;
			it.second.center->connection_count++;
			for(const auto &ju: {it.second.from, it.second.to, it.second.center}) {
				if(ju->layer == 10000) { //none assigned
					ju->layer = it.second.layer;
				}
			}
		}

		auto params = rules.get_parameters();
		for(auto &it: vias) {
			it.second.junction->has_via = true;
			it.second.padstack = *it.second.vpp_padstack;
			ParameterSet ps_via = it.second.parameter_set;
			ps_via[ParameterID::VIA_SOLDER_MASK_EXPANSION] = params->via_solder_mask_expansion;
			it.second.padstack.apply_parameter_set(ps_via);
			it.second.padstack.expand_inner(n_inner_layers);
		}
		for(auto &it: holes) {
			it.second.padstack = *it.second.pool_padstack;
			ParameterSet ps_hole = it.second.parameter_set;
			ps_hole[ParameterID::HOLE_SOLDER_MASK_EXPANSION] = params->hole_solder_mask_expansion;
			it.second.padstack.apply_parameter_set(ps_hole);
			it.second.padstack.expand_inner(n_inner_layers);
			if(it.second.padstack.type == Padstack::Type::HOLE && it.second.net == nullptr) {
				warnings.emplace_back(it.second.placement.shift, "PTH hole without net");
			}
			if(it.second.padstack.type != Padstack::Type::HOLE && it.second.padstack.type != Padstack::Type::MECHANICAL) {
				warnings.emplace_back(it.second.placement.shift, "Illegal padstack type");
			}
		}

		for(const auto &it: junctions) {
			if(it.second.needs_via && !it.second.has_via) {
				warnings.emplace_back(it.second.position, "Junction needs via");
			}
		}

		vacuum_junctions();

		expand_packages();

		for(auto &it: polygons) {
			it.second.usage = nullptr;
		}
		for(auto &it: planes) {
			it.second.polygon->usage = &it.second;
		}


		do {
			//clear net segments and nets assigned to tracks and junctions
			for(auto &it: packages) {
				for(auto &it_pad: it.second.package.pads) {
					it_pad.second.net_segment = UUID();
				}
			}
			for(auto &it: tracks) {
				it.second.update_refs(*this);
				it.second.net = nullptr;
				it.second.net_segment = UUID();
			}
			for(auto &it: junctions) {
				it.second.net = nullptr;
				it.second.net_segment = UUID();
			}
		} while(propagate_net_segments() == false); //run as long as propagate net_segments deletes tracks

		for(auto &it: vias) {
			if(it.second.junction->net == nullptr && !it.second.net_set) {
				warnings.emplace_back(it.second.junction->position, "Via without net");
			}
			else if(it.second.junction->net && it.second.net_set && (it.second.junction->net != it.second.net_set)) {
				warnings.emplace_back(it.second.junction->position, "Via net mismatch");
			}
		}

		update_airwires();
	}

	void Board::expand_packages() {
		auto params = rules.get_parameters();
		ParameterSet pset = {
			{ParameterID::COURTYARD_EXPANSION, params->courtyard_expansion},
			{ParameterID::PASTE_MASK_CONTRACTION, params->paste_mask_contraction},
			{ParameterID::SOLDER_MASK_EXPANSION, params->solder_mask_expansion},
			{ParameterID::HOLE_SOLDER_MASK_EXPANSION, params->hole_solder_mask_expansion},

		};

		for(auto &it: packages) {
			it.second.pool_package = it.second.component->part->package;
			if(it.second.alternate_package) {
				std::set<std::string> pads_from_primary, pads_from_alt;
				for(const auto &it_pad: it.second.pool_package->pads) {
					pads_from_primary.insert(it_pad.second.name);
				}
				bool alt_valid = true;
				for(const auto &it_pad: it.second.alternate_package->pads) {
					if(!pads_from_alt.insert(it_pad.second.name).second) { //duplicate pad name
						alt_valid = false;
					}
				}
				if(!alt_valid || pads_from_alt != pads_from_primary) { //alt pkg isn't pad-equal
					it.second.package = *it.second.pool_package;
					warnings.emplace_back(it.second.placement.shift, "Incompatible alt pkg");
				}
				else {
					it.second.package = *it.second.alternate_package;
					it.second.package.pads.clear(); //need to adjust pad uuids to primary package
					std::map<std::string, UUID> pad_uuids;
					for(const auto &it_pad: it.second.pool_package->pads) {
						assert(pad_uuids.emplace(it_pad.second.name, it_pad.first).second); //no duplicates
					}
					for(const auto &it_pad: it.second.alternate_package->pads) {
						auto uu = pad_uuids.at(it_pad.second.name);
						auto &pad = it.second.package.pads.emplace(uu, it_pad.second).first->second;
						pad.uuid = uu;
					}
				}
			}
			else {
				it.second.package = *it.second.pool_package;
			}
			auto r = it.second.package.apply_parameter_set(pset);
			it.second.placement.mirror = it.second.flip;
			for(auto &it2: it.second.package.pads) {
				it2.second.padstack.expand_inner(n_inner_layers);
			}

			if(it.second.flip) {
				for(auto &it2: it.second.package.lines) {
					flip_package_layer(it2.second.layer);
				}
				for(auto &it2: it.second.package.arcs) {
					flip_package_layer(it2.second.layer);
				}
				for(auto &it2: it.second.package.texts) {
					flip_package_layer(it2.second.layer);
				}
				for(auto &it2: it.second.package.polygons) {
					flip_package_layer(it2.second.layer);
				}
				for(auto &it2: it.second.package.pads) {
					if(it2.second.padstack.type == Padstack::Type::TOP) {
						it2.second.padstack.type = Padstack::Type::BOTTOM;
					}
					else if(it2.second.padstack.type == Padstack::Type::BOTTOM) {
						it2.second.padstack.type = Padstack::Type::TOP;
					}
					for(auto &it3: it2.second.padstack.polygons) {
						flip_package_layer(it3.second.layer);
					}
					for(auto &it3: it2.second.padstack.shapes) {
						flip_package_layer(it3.second.layer);
					}
				}
			}

			it.second.texts.erase(std::remove_if(it.second.texts.begin(), it.second.texts.end(), [this](const auto &a){
				return texts.count(a.uuid) == 0;
			}), it.second.texts.end());

			for(auto &it_text: it.second.package.texts) {
				it_text.second.text = it.second.replace_text(it_text.second.text);
			}
			for(auto it_text: it.second.texts) {
				it_text->text_override = it.second.replace_text(it_text->text, &it_text->overridden);
			}

		}
		//assign nets to pads based on netlist
		for(auto &it: packages) {
			for(auto &it_pad: it.second.package.pads) {
				if(it.second.component->part->pad_map.count(it_pad.first)) {
					const auto &pad_map_item = it.second.component->part->pad_map.at(it_pad.first);
					auto pin_path = UUIDPath<2>(pad_map_item.gate->uuid, pad_map_item.pin->uuid);
					if(it.second.component->connections.count(pin_path)) {
						const auto &conn = it.second.component->connections.at(pin_path);
						it_pad.second.net = conn.net;
					}
					else {
						it_pad.second.net = nullptr;
					}
				}
				else {
					it_pad.second.net = nullptr;
				}
			}
		}
	}

	void Board::disconnect_package(BoardPackage *pkg) {
		std::map<Pad*, Junction*> pad_junctions;
		for(auto &it_track: tracks) {
			Track *tr = &it_track.second;
			//if((line->to_symbol && line->to_symbol->uuid == it.uuid) || (line->from_symbol &&line->from_symbol->uuid == it.uuid)) {
			for(auto it_ft: {&tr->to, &tr->from}) {
				if(it_ft->package == pkg) {
					Junction *j = nullptr;
					if(pad_junctions.count(it_ft->pad)) {
						j = pad_junctions.at(it_ft->pad);
					}
					else {
						auto uu = UUID::random();
						auto x = pad_junctions.emplace(it_ft->pad, &junctions.emplace(uu, uu).first->second);
						j = x.first->second;
					}
					auto c = it_ft->get_position();
					j->position = c;
					it_ft->connect(j);
				}
			}
		}
	}

	void Board::smash_package(BoardPackage *pkg) {
		if(pkg->smashed)
			return;
		pkg->smashed = true;
		for(const auto &it: pkg->pool_package->texts) {
			if(it.second.layer == 20 || it.second.layer==120) { //top or bottom silkscreen
				auto uu = UUID::random();
				auto &x = texts.emplace(uu, uu).first->second;
				x.from_smash = true;
				x.overridden = true;
				x.placement = pkg->placement;
				x.placement.accumulate(it.second.placement);
				x.text = it.second.text;
				x.layer = it.second.layer;
				if(pkg->flip)
					flip_package_layer(x.layer);

				x.size = it.second.size;
				x.width = it.second.width;
				pkg->texts.push_back(&x);
			}
		}
	}

	void Board::unsmash_package(BoardPackage *pkg) {
		if(!pkg->smashed)
			return;
		pkg->smashed = false;
		for(auto &it: pkg->texts) {
			if(it->from_smash) {
				texts.erase(it->uuid); //expand will delete from sym->texts
			}
		}
	}

	void Board::delete_dependants() {
		map_erase_if(vias, [this](auto &it) {return junctions.count(it.second.junction.uuid)==0;});
		map_erase_if(planes, [this](auto &it) {return polygons.count(it.second.polygon.uuid)==0;});
	}


	json Board::serialize() const {
		json j;
		j["type"] = "board";
		j["uuid"] = (std::string)uuid;
		j["block"] = (std::string)block->uuid;
		j["name"] = name;
		j["n_inner_layers"] = n_inner_layers;
		j["rules"] = rules.serialize();
		j["fab_output_settings"] = fab_output_settings.serialize();

		j["polygons"] = json::object();
		for(const auto &it: polygons) {
			j["polygons"][(std::string)it.first] = it.second.serialize();
		}
		j["holes"] = json::object();
		for(const auto &it: holes) {
			j["holes"][(std::string)it.first] = it.second.serialize();
		}
		j["packages"] = json::object();
		for(const auto &it: packages) {
			j["packages"][(std::string)it.first] = it.second.serialize();
		}
		j["junctions"] = json::object();
		for(const auto &it: junctions) {
			j["junctions"][(std::string)it.first] = it.second.serialize();
		}
		j["tracks"] = json::object();
		for(const auto &it: tracks) {
			j["tracks"][(std::string)it.first] = it.second.serialize();
		}
		j["vias"] = json::object();
		for(const auto &it: vias) {
			j["vias"][(std::string)it.first] = it.second.serialize();
		}
		j["texts"] = json::object();
		for(const auto &it: texts) {
			j["texts"][(std::string)it.first] = it.second.serialize();
		}
		j["lines"] = json::object();
		for(const auto &it: lines) {
			j["lines"][(std::string)it.first] = it.second.serialize();
		}
		j["arcs"] = json::object();
		for(const auto &it: arcs) {
			j["arcs"][(std::string)it.first] = it.second.serialize();
		}
		j["planes"] = json::object();
		for(const auto &it: planes) {
			j["planes"][(std::string)it.first] = it.second.serialize();
		}
		j["stackup"] = json::object();
		for(const auto &it: stackup) {
			j["stackup"][std::to_string(it.first)] = it.second.serialize();
		}
		j["dimensions"] = json::object();
		for(const auto &it: dimensions) {
			j["dimensions"][(std::string)it.first] = it.second.serialize();
		}
		return j;
	}
}
