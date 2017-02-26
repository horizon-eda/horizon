#include "core_board.hpp"
#include <algorithm>
#include "util.hpp"
#include "part.hpp"

namespace horizon {
	CoreBoard::CoreBoard(const std::string &board_filename, const std::string &block_filename, const std::string &constraints_filename, const std::string &via_dir, Pool &pool) :
		constraints(Constraints::new_from_file(constraints_filename)),
		via_padstack_provider(via_dir),
		block(Block::new_from_file(block_filename, pool, &constraints)),
		block_work(block),
		brd(Board::new_from_file(board_filename, block, pool, via_padstack_provider)),
		brd_work(brd),
		m_board_filename(board_filename),
		m_block_filename(block_filename),
		m_constraints_filename(constraints_filename),
		m_via_dir(via_dir),
		m_layers({
		{50, {50, "Outline", {.6,.6, 0}}},
		{40, {40, "Top Courtyard", {.5,.5,.5}}},
		{30, {30, "Top Placement", {.5,.5,.5}}},
		{20, {20, "Top Silkscreen", {.9,.9,.9}}},
		{10, {10, "Top Mask", {1,.5,.5}}},
		{0, {0, "Top Copper", {1,0,0}, false, true}},
		{-100, {-100, "Bottom Copper", {0,.5,0}, true, true}},
		{-110, {-110, "Bottom Mask", {.25,.5,.25}, true}},
		{-120, {-120, "Bottom Silkscreen", {.9,.9,.9}, true}},
		{-130, {-130, "Bottom Placement", {.5,.5,.5}}},
		{-140, {-140, "Bottom Courtyard", {.5,.5,.5}}},

	})
	{
		for(unsigned int i = 0; i<brd.n_inner_layers; i++) {
			auto j = i+1;
			m_layers.emplace(std::make_pair(-j, Layer(-j, "Inner "+std::to_string(j), {1,1,0}, false, true)));
		}

		m_pool = &pool;
		brd.block = &block;
		brd_work.block = &block_work;
		brd.update_refs();
		brd_work.update_refs();
		rebuild(true);
	}

	void CoreBoard::reload_netlist() {
		block = Block::new_from_file(m_block_filename, *m_pool, &constraints);
		brd.block = &block;
		brd.update_refs();
		for(auto it=brd.packages.begin(); it!=brd.packages.end();) {
			if(it->second.component == nullptr || it->second.component->part == nullptr) {
				brd.packages.erase(it++);
			}
			else {
				it++;
			}
		}
		brd.update_refs();
		for(auto it=brd.tracks.begin(); it!=brd.tracks.end();) {
			bool del = false;
			for(auto &it_ft: {it->second.from, it->second.to}) {
				if(it_ft.package.uuid) {
					if(brd.packages.count(it_ft.package.uuid) == 0) {
						del = true;
					}
					else {
						if(it_ft.package->component->part->pad_map.count(it_ft.pad.uuid) == 0) {
							del = true;
						}
					}
				}
			}

			if(del) {
				brd.tracks.erase(it++);
			}
			else {
				it++;
			}
		}
		brd.update_refs();

		rebuild(true);

	}

	bool CoreBoard::has_object_type(ObjectType ty) {
		switch(ty) {
			case ObjectType::JUNCTION:
			case ObjectType::POLYGON:
			case ObjectType::HOLE:
			case ObjectType::TRACK:
			case ObjectType::POLYGON_EDGE:
			case ObjectType::POLYGON_VERTEX:
			case ObjectType::POLYGON_ARC_CENTER:
			case ObjectType::TEXT:
			case ObjectType::LINE:
			case ObjectType::BOARD_PACKAGE:
			case ObjectType::VIA:
				return true;
			break;
			default:
				;
		}

		return false;
	}

	std::map<UUID, Polygon> *CoreBoard::get_polygon_map(bool work) {
		auto &p = work?brd_work:brd;
		return &p.polygons;
	}
	std::map<UUID, Hole> *CoreBoard::get_hole_map(bool work) {
		auto &p = work?brd_work:brd;
		return &p.holes;
	}
	std::map<UUID, Junction> *CoreBoard::get_junction_map(bool work) {
		auto &p = work?brd_work:brd;
		return &p.junctions;
	}
	std::map<UUID, Text> *CoreBoard::get_text_map(bool work) {
		auto &p = work?brd_work:brd;
		return &p.texts;
	}

	bool CoreBoard::get_property_bool(const UUID &uu, ObjectType type, ObjectProperty::ID property, bool *handled) {
		bool h = false;
		bool r= Core::get_property_bool(uu, type, property, &h);
		if(h)
			return r;
		switch(type) {
			case ObjectType::BOARD_PACKAGE :
				switch(property) {
					case ObjectProperty::ID::FLIPPED :
						return brd.packages.at(uu).flip;
					default :
						return false;
				}
			break;
			case ObjectType::TRACK :
				switch(property) {
					case ObjectProperty::ID::WIDTH_FROM_NET_CLASS :
						return brd.tracks.at(uu).width_from_net_class;
					default :
						return false;
				}
			break;
			default :
				return false;
		}
		return false;
	}
	void CoreBoard::set_property_bool(const UUID &uu, ObjectType type, ObjectProperty::ID property, bool value, bool *handled) {
		if(tool_is_active())
			return;
		bool h = false;
		Core::set_property_bool(uu, type, property, value, &h);
		if(h)
			return;
		switch(type) {
			case ObjectType::BOARD_PACKAGE :
				switch(property) {
					case ObjectProperty::ID::FLIPPED :
						brd.packages.at(uu).flip = value;
					break;
					default :
						;
				}
			break;
			case ObjectType::TRACK :
				switch(property) {
					case ObjectProperty::ID::WIDTH_FROM_NET_CLASS :
						brd.tracks.at(uu).width_from_net_class = value;
					break;
					default :
						;
				}
			break;
			default :
				;
		}
		rebuild();
		set_needs_save(true);
	}
	int64_t CoreBoard::get_property_int(const UUID &uu, ObjectType type, ObjectProperty::ID property, bool *handled) {
		bool h = false;
		int64_t r= Core::get_property_int(uu, type, property, &h);
		if(h)
			return r;
		switch(type) {
			case ObjectType::TRACK :
				switch(property) {
					case ObjectProperty::ID::WIDTH : {
						auto &tr = brd.tracks.at(uu);
						if(tr.width_from_net_class) {
							if(tr.net) {
								return tr.net->net_class->default_width;
							}
							else {
								return 0;
							}
						}
						else {
							return brd.tracks.at(uu).width;
						}
					} break;
					case ObjectProperty::ID::LAYER :
						return brd.tracks.at(uu).layer;
					default :
						return false;
				}
			break;
			default :
				return false;
		}
		return false;
	}
	void CoreBoard::set_property_int(const UUID &uu, ObjectType type, ObjectProperty::ID property, int64_t value, bool *handled) {
		if(tool_is_active())
			return;
		bool h = false;
		Core::set_property_int(uu, type, property, value, &h);
		if(h)
			return;
		switch(type) {
			case ObjectType::TRACK:
				switch(property) {
					case ObjectProperty::ID::WIDTH : {
						auto &tr = brd.tracks.at(uu);
						if(tr.width_from_net_class)
							return;
						value = std::max((int64_t)0, value);
						if(tr.net) {
							value = std::max((int64_t)tr.net->net_class->min_width, value);
						}

						tr.width = value;
					} break;
					case ObjectProperty::ID::LAYER :
						brd.tracks.at(uu).layer = value;
					break;
					default :
						;
				}
			break;
			default :
				;
		}
		rebuild();
		set_needs_save(true);
	}

	std::string CoreBoard::get_property_string(const UUID &uu, ObjectType type, ObjectProperty::ID property, bool *handled) {
		bool h = false;
		std::string r = Core::get_property_string(uu, type, property, &h);
		if(h)
			return r;
		switch(type) {
			case ObjectType::TRACK :
				switch(property) {
					case ObjectProperty::ID::NAME :
						if(brd.tracks.at(uu).net) {
							return brd.tracks.at(uu).net->name;
						}
						return "<no net>";
					case ObjectProperty::ID::NET_CLASS :
						if(brd.tracks.at(uu).net) {
							return brd.tracks.at(uu).net->net_class->name;
						}
						return "<no net>";
					default :
						return "meh";
				}
			break;
			case ObjectType::BOARD_PACKAGE :
				switch(property) {
					case ObjectProperty::ID::REFDES :
						return brd.packages.at(uu).component->refdes;
					case ObjectProperty::ID::NAME :
						return brd.packages.at(uu).component->part->package->name;
					default :
						return "meh";
				}
			break;

			default :
				return "meh";
		}
		return "meh";
	}

	bool CoreBoard::property_is_settable(const UUID &uu, ObjectType type, ObjectProperty::ID property, bool *handled) {
		bool h = false;
		bool r= Core::property_is_settable(uu, type, property, &h);
		if(h)
			return r;
		switch(type) {
			case ObjectType::TRACK :
				switch(property) {
					case ObjectProperty::ID::WIDTH:
						return !brd.tracks.at(uu).width_from_net_class;
					break;
					default :
						;
				}
			break;
			default :
				;
		}
		return true;
	}

	std::vector<Track*> CoreBoard::get_tracks(bool work) {
		auto &p = work?brd_work:brd;
		std::vector<Track*> r;
		for(auto &it: p.tracks) {
			r.push_back(&it.second);
		}
		return r;
	}

	void CoreBoard::rebuild(bool from_undo) {
		brd.expand();
		brd_work = brd;
		block_work = block;
		brd_work.block = &block_work;
		brd_work.update_refs();
		Core::rebuild(from_undo);
	}

	const std::map<int, Layer> &CoreBoard::get_layers() {
		return m_layers;
	}

	const Board *CoreBoard::get_canvas_data() {
		return &brd_work;
	}

	Board *CoreBoard::get_board(bool work) {
		return work?&brd_work:&brd;
	}

	Constraints *CoreBoard::get_constraints() {
		return &constraints;
	}

	ViaPadstackProvider *CoreBoard::get_via_padstack_provider() {
		return &via_padstack_provider;
	}

	void CoreBoard::commit() {
		brd = brd_work;
		block = block_work;
		brd.block = &block;
		brd_work.block = &block_work;
		brd.update_refs();
		brd_work.update_refs();
		set_needs_save(true);
	}

	void CoreBoard::revert() {
		brd_work = brd;
		block_work = block;
		brd.block = &block;
		brd_work.block = &block_work;
		brd.update_refs();
		brd_work.update_refs();
		reverted = true;
	}

	void CoreBoard::history_push() {
		history.push_back(std::make_unique<CoreBoard::HistoryItem>(block, brd));
		auto x = dynamic_cast<CoreBoard::HistoryItem*>(history.back().get());
		x->brd.block = &x->block;
		x->brd.update_refs();
	}

	void CoreBoard::history_load(unsigned int i) {
		auto x = dynamic_cast<CoreBoard::HistoryItem*>(history.at(history_current).get());
		brd = x->brd;
		block = x->block;
		brd.block = &block;
		brd.update_refs();
		rebuild(true);
	}

	json CoreBoard::get_meta() {
		json j;
		std::ifstream ifs(m_board_filename);
		if(!ifs.is_open()) {
			throw std::runtime_error("file "  +m_board_filename+ " not opened");
		}
		ifs>>j;
		ifs.close();
		if(j.count("_imp")) {
			return j["_imp"];
		}
		return nullptr;
	}

	std::pair<Coordi,Coordi> CoreBoard::get_bbox() {
		Coordi a,b;
		bool found = false;
		for(const auto &it: brd_work.polygons) {
			if(it.second.layer == 50) { //outline
				found = true;
				for(const auto &v: it.second.vertices) {
					a = Coordi::min(a, v.position);
					b = Coordi::max(b, v.position);
				}
			}
		}
		if(!found)
			return {{-10_mm, -10_mm}, {10_mm, 10_mm}};
		return {a,b};
	}

	void CoreBoard::save() {
		auto j = brd.serialize();
		auto save_meta = s_signal_request_save_meta.emit();
		j["_imp"] = save_meta;
		save_json_to_file(m_board_filename, j);
		save_json_to_file(m_block_filename, block.serialize());

		set_needs_save(false);


		//json j = block.serialize();
		//std::cout << std::setw(4) << j << std::endl;
	}
}
