#include "board.hpp"
#include "part.hpp"
#include <list>
#include "util.hpp"

namespace horizon {

	Board::Board(const UUID &uu, const json &j, Block &iblock, Pool &pool, ViaPadstackProvider &vpp):
			uuid(uu),
			block(&iblock),
			name(j.at("name").get<std::string>()),
			n_inner_layers(j.value("n_inner_layers", 0))
		{
		set_n_inner_layers(n_inner_layers);
		if(j.count("polygons")) {
			const json &o = j["polygons"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID(it.key());
				polygons.emplace(std::make_pair(u, Polygon(u, it.value())));
			}
		}
		if(j.count("holes")) {
			const json &o = j["holes"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID(it.key());
				holes.emplace(std::make_pair(u, Hole(u, it.value())));
			}
		}
		if(j.count("packages")) {
			const json &o = j["packages"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				UUID comp_uuid (it.value().at("component").get<std::string>());
				if(block->components.count(comp_uuid) && block->components.at(comp_uuid).part != nullptr) {
					auto u = UUID(it.key());
					packages.emplace(std::make_pair(u, BoardPackage(u, it.value(), *block, pool)));
				}
			}
		}
		if(j.count("junctions")) {
			const json &o = j["junctions"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID(it.key());
				junctions.emplace(std::make_pair(u, Junction(u, it.value())));
			}
		}
		if(j.count("tracks")) {
			const json &o = j["tracks"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				bool valid = true;
				for(const auto &it_ft: {it.value().at("from"), it.value().at("to")}) {
					if(it_ft.at("pad") != nullptr) {
						UUIDPath<2> path(it_ft.at("pad").get<std::string>());
						valid = packages.count(path.at(0));
						if(valid) {
							auto &pkg = packages.at(path.at(0));
							if(pkg.component->part->pad_map.count(path.at(1))==0) {
								valid = false;
							}
						}
					}
				}
				if(valid) {
					auto u = UUID(it.key());
					tracks.emplace(std::make_pair(u, Track(u, it.value(), *this)));
				}
			}
		}
		if(j.count("vias")) {
			const json &o = j["vias"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID(it.key());
				vias.emplace(std::piecewise_construct, std::forward_as_tuple(u), std::forward_as_tuple(u, it.value(), *this, vpp));
			}
		}
		if(j.count("texts")) {
			const json &o = j["texts"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID(it.key());
				texts.emplace(std::make_pair(u, Text(u, it.value())));
			}
		}
		if(j.count("lines")) {
			const json &o = j["lines"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID(it.key());
				lines.emplace(std::make_pair(u, Line(u, it.value(), *this)));
			}
		}
		if(j.count("planes")) {
			const json &o = j["planes"];
			for (auto it = o.cbegin(); it != o.cend(); ++it) {
				auto u = UUID(it.key());
				planes.emplace(std::piecewise_construct, std::forward_as_tuple(u), std::forward_as_tuple(u, it.value(), *this));
			}
		}
		if(j.count("rules")) {
			rules.load_from_json(j.at("rules"));
			rules.cleanup(block);
		}
		if(j.count("fab_output_settings")) {
			fab_output_settings = FabOutputSettings(j.at("fab_output_settings"));
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
		rules.add_rule(RuleID::TRACK_WIDTH);
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
		planes(brd.planes),
		warnings(brd.warnings),
		rules(brd.rules),
		fab_output_settings(brd.fab_output_settings),
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
		planes = brd.planes;
		warnings = brd.warnings;
		rules = brd.rules;
		fab_output_settings = brd.fab_output_settings;
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
		}
		for(auto &it: junctions) {
			it.second.net.update(block->nets);
		}
		for(auto &it: lines) {
			auto &line = it.second;
			line.to = &junctions.at(line.to.uuid);
			line.from = &junctions.at(line.from.uuid);
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
		for(unsigned int i = 0; i<n_inner_layers; i++) {
			auto j = i+1;
			layers.emplace(std::make_pair(-j, Layer(-j, "Inner "+std::to_string(j), {1,1,0}, false, true)));
		}
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
			if(run == false) { //no pad needing assigment found, done
				break;
			}
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
		}

		for(auto &it: vias) {
			it.second.junction->has_via = true;
			it.second.padstack = *it.second.vpp_padstack;
			it.second.padstack.apply_parameter_set(it.second.parameter_set);
			it.second.padstack.expand_inner(n_inner_layers);
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
		update_airwires();
	}

	void Board::expand_packages() {
		auto params = rules.get_parameters();
		ParameterSet pset = {
			{ParameterID::COURTYARD_EXPANSION, params->courtyard_expansion},
			{ParameterID::PASTE_MASK_CONTRACTION, params->paste_mask_contraction},
			{ParameterID::SOLDER_MASK_EXPANSION, params->solder_mask_expansion},

		};

		for(auto &it: packages) {
			it.second.pool_package = it.second.component->part->package;
			it.second.package = *it.second.pool_package;
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
		j["planes"] = json::object();
		for(const auto &it: planes) {
			j["planes"][(std::string)it.first] = it.second.serialize();
		}
		return j;
	}
}
