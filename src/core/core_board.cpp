#include "core_board.hpp"
#include "core_properties.hpp"
#include <algorithm>
#include "util/util.hpp"
#include "pool/part.hpp"
#include "board/board_layers.hpp"

namespace horizon {
	CoreBoard::CoreBoard(const std::string &board_filename, const std::string &block_filename, const std::string &via_dir, Pool &pool) :
		via_padstack_provider(via_dir, pool),
		block(Block::new_from_file(block_filename, pool)),
		brd(Board::new_from_file(board_filename, block, pool, via_padstack_provider)),
		rules(brd.rules),
		fab_output_settings(brd.fab_output_settings),
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

		rebuild();
	}

	bool CoreBoard::has_object_type(ObjectType ty) {
		switch(ty) {
			case ObjectType::JUNCTION:
			case ObjectType::POLYGON:
			case ObjectType::BOARD_HOLE:
			case ObjectType::TRACK:
			case ObjectType::POLYGON_EDGE:
			case ObjectType::POLYGON_VERTEX:
			case ObjectType::POLYGON_ARC_CENTER:
			case ObjectType::TEXT:
			case ObjectType::LINE:
			case ObjectType::ARC:
			case ObjectType::BOARD_PACKAGE:
			case ObjectType::VIA:
			case ObjectType::DIMENSION:
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
	std::map<UUID, Junction> *CoreBoard::get_junction_map(bool work) {
		return &brd.junctions;
	}
	std::map<UUID, Text> *CoreBoard::get_text_map(bool work) {
		return &brd.texts;
	}
	std::map<UUID, Line> *CoreBoard::get_line_map(bool work) {
		return &brd.lines;
	}
	std::map<UUID, Arc> *CoreBoard::get_arc_map(bool work) {
		return &brd.arcs;
	}
	std::map<UUID, Dimension> *CoreBoard::get_dimension_map() {
		return &brd.dimensions;
	}

	bool CoreBoard::get_property(ObjectType type, const UUID &uu, ObjectProperty::ID property, PropertyValue &value) {
		if(Core::get_property(type, uu, property, value))
			return true;
		switch(type) {
			case ObjectType::BOARD_PACKAGE : {
				auto pkg = &brd.packages.at(uu);
				switch(property) {
					case ObjectProperty::ID::FLIPPED :
						dynamic_cast<PropertyValueBool&>(value).value = pkg->flip;
						return true;

					case ObjectProperty::ID::REFDES :
						dynamic_cast<PropertyValueString&>(value).value = pkg->component->refdes;
						return true;

					case ObjectProperty::ID::NAME :
						dynamic_cast<PropertyValueString&>(value).value = pkg->package.name;
						return true;

					case ObjectProperty::ID::VALUE :
						dynamic_cast<PropertyValueString&>(value).value = pkg->component->part->get_value();
						return true;

					case ObjectProperty::ID::MPN :
						dynamic_cast<PropertyValueString&>(value).value = pkg->component->part->get_MPN();
						return true;

					case ObjectProperty::ID::ALTERNATE_PACKAGE :
						dynamic_cast<PropertyValueUUID&>(value).value = pkg->alternate_package?pkg->alternate_package->uuid:UUID();
						return true;

					case ObjectProperty::ID::POSITION_X :
					case ObjectProperty::ID::POSITION_Y :
					case ObjectProperty::ID::ANGLE :
						get_placement(pkg->placement, value, property);
						return true;

					default :
						return false;
				}

			} break;

			case ObjectType::TRACK : {
				auto track = &brd.tracks.at(uu);
				switch(property) {
					case ObjectProperty::ID::WIDTH_FROM_RULES :
						dynamic_cast<PropertyValueBool&>(value).value = track->width_from_rules;
						return true;

					case ObjectProperty::ID::LOCKED :
						dynamic_cast<PropertyValueBool&>(value).value = track->locked;
						return true;

					case ObjectProperty::ID::WIDTH :
						dynamic_cast<PropertyValueInt&>(value).value = track->width;
						return true;

					case ObjectProperty::ID::LAYER :
						dynamic_cast<PropertyValueInt&>(value).value = track->layer;
						return true;

					case ObjectProperty::ID::NAME :
						dynamic_cast<PropertyValueString&>(value).value = track->net?(track->net->name):"<no net>";
						return true;

					case ObjectProperty::ID::NET_CLASS :
						dynamic_cast<PropertyValueString&>(value).value = track->net?(track->net->net_class->name):"<no net>";
						return true;

					default :
						return false;
				}
			}break;

			case ObjectType::VIA : {
				auto via = &brd.vias.at(uu);
				switch(property) {
					case ObjectProperty::ID::FROM_RULES :
						dynamic_cast<PropertyValueBool&>(value).value = via->from_rules;
						return true;

					case ObjectProperty::ID::LOCKED :
						dynamic_cast<PropertyValueBool&>(value).value = via->locked;
						return true;

					case ObjectProperty::ID::NAME : {
						std::string s;
						if(via->junction->net) {
							s = via->junction->net->name;
							if(via->net_set)
								s += " (set)";
						}
						else {
							s = "<no net>";
						}
						dynamic_cast<PropertyValueString&>(value).value = s;
						return true;
					}


					default :
						return false;
				}
			} break;

			case ObjectType::PLANE : {
				auto plane = &brd.planes.at(uu);
				switch(property) {
					case ObjectProperty::ID::FROM_RULES :
						dynamic_cast<PropertyValueBool&>(value).value = plane->from_rules;
						return true;

					case ObjectProperty::ID::WIDTH :
						dynamic_cast<PropertyValueInt&>(value).value = plane->settings.min_width;
						return true;

					case ObjectProperty::ID::NAME :
						dynamic_cast<PropertyValueString&>(value).value = plane->net?(plane->net->name):"<no net>";
						return true;

					default :
						return false;
				}
			} break;

			case ObjectType::BOARD_HOLE : {
				auto hole = &brd.holes.at(uu);
				switch(property) {
					case ObjectProperty::ID::NAME :
						dynamic_cast<PropertyValueString&>(value).value = hole->pool_padstack->name;
						return true;

					case ObjectProperty::ID::VALUE : {
						std::string net = "<no net>";
						if(hole->net)
							net = hole->net->name;
						dynamic_cast<PropertyValueString&>(value).value = net;
						return true;
					}

					case ObjectProperty::ID::POSITION_X :
					case ObjectProperty::ID::POSITION_Y :
					case ObjectProperty::ID::ANGLE :
						get_placement(hole->placement, value, property);
						return true;

					case ObjectProperty::ID::PAD_TYPE : {
						const auto ps = brd.holes.at(uu).pool_padstack;
						std::string pad_type;
						switch(ps->type) {
							case Padstack::Type::MECHANICAL:	pad_type = "Mechanical (NPTH)"; break;
							case Padstack::Type::HOLE:			pad_type = "Hole (PTH)"; break;
							default:							pad_type = "Invalid";
						}
						dynamic_cast<PropertyValueString&>(value).value = pad_type;
						return true;
					} break;

					default :
						return false;
				}
			} break;

			default :
				return false;
		}
	}

	bool CoreBoard::set_property(ObjectType type, const UUID &uu, ObjectProperty::ID property, const PropertyValue &value) {
		if(Core::set_property(type, uu, property, value))
			return true;
		switch(type) {
			case ObjectType::BOARD_PACKAGE : {
				auto pkg = &brd.packages.at(uu);
				switch(property) {
					case ObjectProperty::ID::FLIPPED :
						pkg->flip = dynamic_cast<const PropertyValueBool&>(value).value;
					break;

					case ObjectProperty::ID::ALTERNATE_PACKAGE : {
						const auto alt_uuid = dynamic_cast<const PropertyValueUUID&>(value).value;
						if(!alt_uuid) {
							pkg->alternate_package = nullptr;
						}
						else if(m_pool->get_alternate_packages(pkg->pool_package->uuid).count(alt_uuid) != 0) { //see if really an alt package for pkg
							pkg->alternate_package = m_pool->get_package(alt_uuid);
						}
					} break;

					case ObjectProperty::ID::POSITION_X :
					case ObjectProperty::ID::POSITION_Y :
					case ObjectProperty::ID::ANGLE :
						set_placement(pkg->placement, value, property);
					break;

					default :
						return false;
				}
			} break;

			case ObjectType::TRACK : {
				auto track = &brd.tracks.at(uu);
				switch(property) {
					case ObjectProperty::ID::WIDTH_FROM_RULES :
						track->width_from_rules = dynamic_cast<const PropertyValueBool&>(value).value;
					break;

					case ObjectProperty::ID::LOCKED :
						track->locked = dynamic_cast<const PropertyValueBool&>(value).value;
					break;

					case ObjectProperty::ID::WIDTH :
						track->width = dynamic_cast<const PropertyValueInt&>(value).value;
					break;

					case ObjectProperty::ID::LAYER :
						track->layer = dynamic_cast<const PropertyValueInt&>(value).value;
					break;

					default :
						return false;
				}
			}break;

			case ObjectType::VIA : {
				auto via = &brd.vias.at(uu);
				switch(property) {
					case ObjectProperty::ID::FROM_RULES :
						via->from_rules = dynamic_cast<const PropertyValueBool&>(value).value;
					break;

					case ObjectProperty::ID::LOCKED :
						via->locked = dynamic_cast<const PropertyValueBool&>(value).value;
					break;

					default :
						return false;
				}
			} break;

			case ObjectType::PLANE : {
				auto plane = &brd.planes.at(uu);
				switch(property) {
					case ObjectProperty::ID::FROM_RULES :
						plane->from_rules = dynamic_cast<const PropertyValueBool&>(value).value;
					break;

					case ObjectProperty::ID::WIDTH :
						if(!plane->from_rules)
							plane->settings.min_width = dynamic_cast<const PropertyValueInt&>(value).value;
					break;

					default :
						return false;
				}
			} break;

			case ObjectType::BOARD_HOLE : {
				auto hole = &brd.holes.at(uu);
				switch(property) {
					case ObjectProperty::ID::POSITION_X :
					case ObjectProperty::ID::POSITION_Y :
					case ObjectProperty::ID::ANGLE :
						set_placement(hole->placement, value, property);
					break;

					default:
						return false;
				}
			} break;

			default :
				return false;
		}
		if(!property_transaction) {
			rebuild(false);
			set_needs_save(true);
		}
		return true;
	}

	bool CoreBoard::get_property_meta(ObjectType type, const UUID &uu, ObjectProperty::ID property, PropertyMeta &meta) {
		if(Core::get_property_meta(type, uu, property, meta))
			return true;
		switch(type) {
			case ObjectType::BOARD_PACKAGE : {
				auto pkg = &brd.packages.at(uu);
				switch(property) {
					case ObjectProperty::ID::ALTERNATE_PACKAGE : {
						PropertyMetaNetClasses &m = dynamic_cast<PropertyMetaNetClasses&>(meta);
						m.net_classes.clear();
						m.net_classes.emplace(UUID(), pkg->pool_package->name + " (default)");
						for(const auto &it: m_pool->get_alternate_packages(pkg->pool_package->uuid)) {
							m.net_classes.emplace(it, m_pool->get_package(it)->name);
						}
						return true;
					}

					default :
						return false;
				}
			}break;

			case ObjectType::TRACK : {
				auto track = &brd.tracks.at(uu);
				switch(property) {
					case ObjectProperty::ID::WIDTH :
						meta.is_settable = !track->width_from_rules;
						return true;

					case ObjectProperty::ID::LAYER :
						layers_to_meta(meta);
						return true;

					default :
						return false;
				}
			}break;

			case ObjectType::PLANE : {
				auto plane = &brd.planes.at(uu);
				switch(property) {
					case ObjectProperty::ID::WIDTH :
						meta.is_settable = !plane->from_rules;
						return true;

					default :
						return false;
				}
			}break;

			default:
				return false;
		}
	}

	std::string CoreBoard::get_display_name(ObjectType type, const UUID &uu) {
		switch(type) {
			case ObjectType::BOARD_PACKAGE :
				return brd.packages.at(uu).component->refdes;

			case ObjectType::TRACK : {
				const auto &tr = brd.tracks.at(uu);
				return tr.net?tr.net->name:"";
			}

			case ObjectType::VIA : {
				const auto ju= brd.vias.at(uu).junction;
				return ju->net?ju->net->name:"";
			}

			default :
				return Core::get_display_name(type, uu);
		}
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

	void CoreBoard::update_rules() {
		brd.rules = rules;
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
		brd.fab_output_settings = fab_output_settings;
		auto j = brd.serialize();
		auto save_meta = s_signal_request_save_meta.emit();
		j["_imp"] = save_meta;
		save_json_to_file(m_board_filename, j);

		set_needs_save(false);


		//json j = block.serialize();
		//std::cout << std::setw(4) << j << std::endl;
	}
}
