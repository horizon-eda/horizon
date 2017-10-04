#include "core_board.hpp"
#include <algorithm>
#include "util.hpp"
#include "part.hpp"
#include "board_layers.hpp"

namespace horizon {
	CoreBoard::CoreBoard(const std::string &board_filename, const std::string &block_filename, const std::string &via_dir, Pool &pool) :
		via_padstack_provider(via_dir, pool),
		block(Block::new_from_file(block_filename, pool)),
		brd(Board::new_from_file(board_filename, block, pool, via_padstack_provider)),
		rules(brd.rules),
		m_board_filename(board_filename),
		m_block_filename(block_filename),
		m_via_dir(via_dir)
	{
		m_pool = &pool;
		rebuild();
	}

	void CoreBoard::reload_netlist() {
		block = Block::new_from_file(m_block_filename, *m_pool);
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
		rules.cleanup(&block);

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
		return &brd.polygons;
	}
	std::map<UUID, Hole> *CoreBoard::get_hole_map(bool work) {
		return &brd.holes;
	}
	std::map<UUID, Junction> *CoreBoard::get_junction_map(bool work) {
		return &brd.junctions;
	}
	std::map<UUID, Text> *CoreBoard::get_text_map(bool work) {
		return &brd.texts;
	}
	std::map<UUID, Line> *CoreBoard::get_line_map(bool work) {
		return &brd.lines;
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
					case ObjectProperty::ID::WIDTH_FROM_RULES :
						return brd.tracks.at(uu).width_from_rules;
					default :
						return false;
				}
			break;
			case ObjectType::VIA :
				switch(property) {
					case ObjectProperty::ID::FROM_RULES :
						return brd.vias.at(uu).from_rules;
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
					case ObjectProperty::ID::WIDTH_FROM_RULES :
						brd.tracks.at(uu).width_from_rules = value;
					break;
					default :
						;
				}
			break;
			case ObjectType::VIA :
				switch(property) {
					case ObjectProperty::ID::FROM_RULES :
						brd.vias.at(uu).from_rules = value;
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
						return brd.tracks.at(uu).width;
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
						if(tr.width_from_rules)
							return;
						value = std::max((int64_t)0, value);
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
					case ObjectProperty::ID::VALUE :
						return brd.packages.at(uu).component->part->get_value();
					case ObjectProperty::ID::MPN :
						return brd.packages.at(uu).component->part->get_MPN();
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
						return !brd.tracks.at(uu).width_from_rules;
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
		std::vector<Track*> r;
		for(auto &it: brd.tracks) {
			r.push_back(&it.second);
		}
		return r;
	}

	std::vector<Line*> CoreBoard::get_lines(bool work) {
		std::vector<Line*> r;
		for(auto &it: brd.lines) {
			r.push_back(&it.second);
		}
		return r;
	}

	void CoreBoard::rebuild(bool from_undo) {
		brd.expand();
		Core::rebuild(from_undo);
	}

	LayerProvider *CoreBoard::get_layer_provider() {
		return &brd;
	}

	const Board *CoreBoard::get_canvas_data() {
		return &brd;
	}

	Board *CoreBoard::get_board(bool work) {
		return &brd;
	}

	Block *CoreBoard::get_block(bool work) {
		return get_board(work)->block;
	}

	Rules *CoreBoard::get_rules() {
		return &rules;
	}

	ViaPadstackProvider *CoreBoard::get_via_padstack_provider() {
		return &via_padstack_provider;
	}

	void CoreBoard::commit() {
		set_needs_save(true);
	}

	void CoreBoard::revert() {
		history_load(history_current);
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
		for(const auto &it: brd.polygons) {
			if(it.second.layer == BoardLayers::L_OUTLINE) { //outline
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
		brd.rules = rules;
		auto j = brd.serialize();
		auto save_meta = s_signal_request_save_meta.emit();
		j["_imp"] = save_meta;
		save_json_to_file(m_board_filename, j);

		set_needs_save(false);


		//json j = block.serialize();
		//std::cout << std::setw(4) << j << std::endl;
	}
}
