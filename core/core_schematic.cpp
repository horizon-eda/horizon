#include "core_schematic.hpp"
#include <algorithm>
#include "part.hpp"
#include "util.hpp"

namespace horizon {
	CoreSchematic::CoreSchematic(const std::string &schematic_filename, const std::string &block_filename, Pool &pool) :
		block(Block::new_from_file(block_filename, pool)),
		sch(Schematic::new_from_file(schematic_filename, block, pool)),
		rules(sch.rules),
		m_schematic_filename(schematic_filename),
		m_block_filename(block_filename)
	{
		auto x = std::find_if(sch.sheets.cbegin(), sch.sheets.cend(), [](const auto &a){return a.second.index == 1;});
		assert(x!=sch.sheets.cend());
		sheet_uuid = x->first;
		m_pool = &pool;
		rebuild();
	}

	Junction *CoreSchematic::get_junction(const UUID &uu, bool work) {
		auto &sheet = sch.sheets.at(sheet_uuid);
		return &sheet.junctions.at(uu);
	}
	SchematicSymbol *CoreSchematic::get_schematic_symbol(const UUID &uu, bool work) {
		auto &sheet = sch.sheets.at(sheet_uuid);
		return &sheet.symbols.at(uu);
	}
	Text *CoreSchematic::get_text(const UUID &uu, bool work) {
		auto &sheet = sch.sheets.at(sheet_uuid);
		return &sheet.texts.at(uu);
	}
	Schematic *CoreSchematic::get_schematic(bool work) {
		return &sch;
	}

	Block *CoreSchematic::get_block(bool work) {
		return get_schematic(work)->block;
	}

	LayerProvider *CoreSchematic::get_layer_provider() {
		return get_sheet();
	}

	Sheet *CoreSchematic::get_sheet(bool work) {
		return &sch.sheets.at(sheet_uuid);
	}
	Line *CoreSchematic::get_line(const UUID &uu, bool work) {
		return nullptr;
	}
	Arc *CoreSchematic::get_arc(const UUID &uu, bool work) {
		return nullptr;
	}

	Junction *CoreSchematic::insert_junction(const UUID &uu, bool work) {
		auto &sheet = sch.sheets.at(sheet_uuid);
		auto x = sheet.junctions.emplace(std::make_pair(uu, uu));
		return &(x.first->second);
	}

	LineNet *CoreSchematic::insert_line_net(const UUID &uu, bool work) {
		auto &sheet = sch.sheets.at(sheet_uuid);
		auto x = sheet.net_lines.emplace(std::make_pair(uu, uu));
		return &(x.first->second);
		}

	void CoreSchematic::delete_junction(const UUID &uu, bool work) {
		auto &sheet = sch.sheets.at(sheet_uuid);
		sheet.junctions.erase(uu);
	}
	void CoreSchematic::delete_line_net(const UUID &uu, bool work) {
		auto &sheet = sch.sheets.at(sheet_uuid);
		sheet.net_lines.erase(uu);
	}
	void CoreSchematic::delete_schematic_symbol(const UUID &uu, bool work) {
		auto &sheet = sch.sheets.at(sheet_uuid);
		sheet.symbols.erase(uu);
	}
	SchematicSymbol *CoreSchematic::insert_schematic_symbol(const UUID &uu, const Symbol *sym, bool work) {
		auto &sheet = sch.sheets.at(sheet_uuid);
		auto x = sheet.symbols.emplace(std::make_pair(uu, SchematicSymbol{uu, sym}));
		return &(x.first->second);
		return nullptr;
	}

	Line *CoreSchematic::insert_line(const UUID &uu, bool work) {
		return nullptr;
	}
	void CoreSchematic::delete_line(const UUID &uu, bool work) {
		return;
	}

	Arc *CoreSchematic::insert_arc(const UUID &uu, bool work) {
		return nullptr;
	}
	void CoreSchematic::delete_arc(const UUID &uu, bool work) {
		return;
	}

	std::vector<LineNet*> CoreSchematic::get_net_lines(bool work) {
		auto &sheet = sch.sheets.at(sheet_uuid);
		std::vector<LineNet*> r;
		for(auto &it: sheet.net_lines) {
			r.push_back(&it.second);
		}
		return r;
	}
	std::vector<NetLabel*> CoreSchematic::get_net_labels(bool work) {
		auto &sheet = sch.sheets.at(sheet_uuid);
		std::vector<NetLabel*> r;
		for(auto &it: sheet.net_labels) {
			r.push_back(&it.second);
		}
		return r;
	}

	std::vector<Line*> CoreSchematic::get_lines(bool work) {
		std::vector<Line*> r;
		return r;
	}

	std::vector<Arc*> CoreSchematic::get_arcs(bool work) {
		std::vector<Arc*> r;
		return r;
	}

	void CoreSchematic::delete_text(const UUID &uu, bool work) {
		auto &sheet = sch.sheets.at(sheet_uuid);
		sheet.texts.erase(uu);
	}
	Text *CoreSchematic::insert_text(const UUID &uu, bool work) {
		auto &sheet = sch.sheets.at(sheet_uuid);
		auto x = sheet.texts.emplace(std::make_pair(uu, uu));
		return &(x.first->second);
	}

	bool CoreSchematic::has_object_type(ObjectType ty) {
		switch(ty) {
			case ObjectType::JUNCTION:
			case ObjectType::SCHEMATIC_SYMBOL:
			case ObjectType::BUS_LABEL:
			case ObjectType::BUS_RIPPER:
			case ObjectType::NET_LABEL:
			case ObjectType::LINE_NET:
			case ObjectType::POWER_SYMBOL:
			case ObjectType::TEXT:
				return true;
			break;
			default:
				;
		}

		return false;
	}

	Rules *CoreSchematic::get_rules() {
		return &rules;
	}


	std::string CoreSchematic::get_property_string(const UUID &uu, ObjectType type, ObjectProperty::ID property, bool *handled) {
		bool h = false;
		std::string r= Core::get_property_string(uu, type, property, &h);
		if(h)
			return r;
		auto &sheet = sch.sheets.at(sheet_uuid);
		switch(type) {
			case ObjectType::NET :
				switch(property) {
					case ObjectProperty::ID::NAME :
						return block.nets.at(uu).name;
					case ObjectProperty::ID::NET_CLASS :
						return block.nets.at(uu).net_class->uuid;
					case ObjectProperty::ID::DIFFPAIR : {
						auto &net = block.nets.at(uu);
						if(net.diffpair) {
							return (net.diffpair_master?"Master: ":"Slave: ")+net.diffpair->name ;
						}
						else {
							return "None";
						}
					}
					default :
						return "<invalid prop>";
				}
			break;
			case ObjectType::NET_LABEL :
				switch(property) {
					case ObjectProperty::ID::NAME :
						if(sheet.net_labels.at(uu).junction->net)
							return sheet.net_labels.at(uu).junction->net->name;
						else
							return "<no net>";
					default :
						return "<invalid prop>";
				}
			break;
			case ObjectType::COMPONENT :
				switch(property) {
					case ObjectProperty::ID::REFDES :
						return block.components.at(uu).refdes;
					case ObjectProperty::ID::VALUE :
						if(block.components.at(uu).part)
							return block.components.at(uu).part->get_value();
						else
							return block.components.at(uu).value;
					case ObjectProperty::ID::MPN :
						if(block.components.at(uu).part)
							return block.components.at(uu).part->get_MPN();
						else
							return "<no part>";
					default :
						return "<invalid prop>";
				}
			break;

			default :
				return "<invalid type>";
		}
		return "<meh>";
	}
	void CoreSchematic::set_property_string(const UUID &uu, ObjectType type, ObjectProperty::ID property, const std::string &value, bool *handled) {
		if(tool_is_active())
			return;
		bool h = false;
		Core::set_property_string(uu, type, property, value, &h);
		if(h)
			return;
		switch(type) {
			case ObjectType::COMPONENT :
				switch(property) {
					case ObjectProperty::ID::REFDES :
						block.components.at(uu).refdes = value;
					break;
					case ObjectProperty::ID::VALUE :
						if((block.components.at(uu).part))
							return;
						block.components.at(uu).value = value;
					break;
					default :
						;
				}
			break;
			case ObjectType::NET :
				switch(property) {
					case ObjectProperty::ID::NAME :
						block.nets.at(uu).name = value;
					break;
					case ObjectProperty::ID::NET_CLASS : {
						UUID nc = value;
						block.nets.at(uu).net_class = &block.net_classes.at(nc);
					}break;
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

	void CoreSchematic::set_property_bool(const UUID &uu, ObjectType type, ObjectProperty::ID property, bool value, bool *handled) {
		if(tool_is_active())
			return;
		bool h = false;
		Core::set_property_bool(uu, type, property, value, &h);
		if(h)
			return;
		auto &sheet = sch.sheets.at(sheet_uuid);
		switch(type) {
			case ObjectType::NET :
				switch(property) {
					case ObjectProperty::ID::IS_POWER :
						if(block.nets.at(uu).is_power_forced)
							return;
						if(block.nets.at(uu).is_power == value)
							return;
						block.nets.at(uu).is_power = value;
					break;
					default :
						;
				}
			break;
			case ObjectType::NET_LABEL :
				switch(property) {
					case ObjectProperty::ID::OFFSHEET_REFS :
						if(sheet.net_labels.at(uu).offsheet_refs == value)
							return;
						sheet.net_labels.at(uu).offsheet_refs = value;
					break;
					default :
						;
				}
			break;
			case ObjectType::SCHEMATIC_SYMBOL :
				switch(property) {
					case ObjectProperty::ID::DISPLAY_DIRECTIONS :
						if(sheet.symbols.at(uu).display_directions == value)
							return;
						sheet.symbols.at(uu).display_directions = value;
					break;
					default :
						;
				}
			break;

			default:
				;
		}
		rebuild();
		set_needs_save(true);
	}

	bool CoreSchematic::get_property_bool(const UUID &uu, ObjectType type, ObjectProperty::ID property, bool *handled) {
		bool h = false;
		bool r= Core::get_property_bool(uu, type, property, &h);
		if(h)
			return r;
		auto &sheet = sch.sheets.at(sheet_uuid);
		switch(type) {
			case ObjectType::NET :
				switch(property) {
					case ObjectProperty::ID::IS_POWER :
						return block.nets.at(uu).is_power;
					default :
						;
				}
			break;
			case ObjectType::NET_LABEL :
				switch(property) {
					case ObjectProperty::ID::OFFSHEET_REFS :
						return sheet.net_labels.at(uu).offsheet_refs;
					default :
						;
				}
			break;
			case ObjectType::SCHEMATIC_SYMBOL :
				switch(property) {
					case ObjectProperty::ID::DISPLAY_DIRECTIONS :
						return sheet.symbols.at(uu).display_directions;
					default :
						;
				}
			break;

			default :
				;
		}
		return false;
	}
	int64_t CoreSchematic::get_property_int(const UUID &uu, ObjectType type, ObjectProperty::ID property, bool *handled) {
		bool h = false;
		int64_t r= Core::get_property_int(uu, type, property, &h);
		if(h)
			return r;
		auto &sheet = sch.sheets.at(sheet_uuid);
		switch(type) {
			case ObjectType::NET_LABEL :
				switch(property) {
					case ObjectProperty::ID::SIZE:
						return sheet.net_labels.at(uu).size;
					default :
						return 0;
				}
			break;

			default :
				return 0;
		}
	}
	void CoreSchematic::set_property_int(const UUID &uu, ObjectType type, ObjectProperty::ID property, int64_t value, bool *handled) {
		if(tool_is_active())
			return;
		bool h = false;
		Core::set_property_int(uu, type, property, value, &h);
		if(h)
			return;
		auto &sheet = sch.sheets.at(sheet_uuid);
		int64_t newv;
		switch(type) {
			case ObjectType::NET_LABEL :
				switch(property) {
					case ObjectProperty::ID::SIZE:
						newv = std::max(value, (int64_t)500000);
						if(newv == (int64_t)sheet.net_labels.at(uu).size)
							return;
						sheet.net_labels.at(uu).size = newv;
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

	bool CoreSchematic::property_is_settable(const UUID &uu, ObjectType type, ObjectProperty::ID property, bool *handled) {
		bool h = false;
		bool r= Core::property_is_settable(uu, type, property, &h);
		if(h)
			return r;
		switch(type) {
			case ObjectType::NET :
				switch(property) {
					case ObjectProperty::ID::IS_POWER:
						return !block.nets.at(uu).is_power_forced;
					break;
					default :
						;
				}
			break;
			case ObjectType::COMPONENT :
				switch(property) {
					case ObjectProperty::ID::VALUE:
						return block.components.at(uu).part==nullptr;
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

	void CoreSchematic::add_sheet() {
		auto uu = UUID::random();
		auto sheet_max = std::max_element(sch.sheets.begin(), sch.sheets.end(),
			[](const auto &p1, const auto &p2) {return p1.second.index < p2.second.index; }
		);
		auto *sheet = &sch.sheets.emplace(uu, uu).first->second;
		sheet->index = sheet_max->second.index+1;
		sheet->name = "sheet "+std::to_string(sheet->index);
		rebuild();
	}

	void CoreSchematic::delete_sheet(const UUID &uu) {
		if(sch.sheets.size() <= 1)
			return;
		if(sch.sheets.at(uu).symbols.size()>0) //only delete empty sheets
			return;
		auto deleted_index = sch.sheets.at(uu).index;
		sch.sheets.erase(uu);
		for(auto &it: sch.sheets) {
			if(it.second.index > deleted_index) {
				it.second.index--;
			}
		}
		if(sheet_uuid == uu) {//deleted current sheet
			auto x = std::find_if(sch.sheets.begin(), sch.sheets.end(), [](auto e){return e.second.index == 1;});
			sheet_uuid = x->first;
		}
		rebuild();
	}

	void CoreSchematic::set_sheet(const UUID &uu) {
		if(tool_is_active())
			return;
		if(sch.sheets.count(uu) == 0)
			return;
		sheet_uuid = uu;
	}

	void CoreSchematic::rebuild(bool from_undo) {
		sch.expand();
		Core::rebuild(from_undo);
	}

	const Sheet *CoreSchematic::get_canvas_data() {
		return &sch.sheets.at(sheet_uuid);
	}

	void CoreSchematic::commit() {
		set_needs_save(true);
	}

	void CoreSchematic::revert() {
		history_load(history_current);
		reverted = true;
	}

	void CoreSchematic::history_push() {
		history.push_back(std::make_unique<CoreSchematic::HistoryItem>(block, sch));
		auto x = dynamic_cast<CoreSchematic::HistoryItem*>(history.back().get());
		x->sch.block = &x->block;
		x->sch.update_refs();
	}

	void CoreSchematic::history_load(unsigned int i) {
		auto x = dynamic_cast<CoreSchematic::HistoryItem*>(history.at(history_current).get());
		sch = x->sch;
		block = x->block;
		sch.block = &block;
		sch.update_refs();
		s_signal_rebuilt.emit();
	}


	std::pair<Coordi,Coordi> CoreSchematic::get_bbox() {
		return get_sheet()->frame.get_bbox();
	}

	void CoreSchematic::save() {
		sch.rules = rules;
		save_json_to_file(m_schematic_filename, sch.serialize());
		save_json_to_file(m_block_filename, block.serialize());
		set_needs_save(false);


		//json j = block.serialize();
		//std::cout << std::setw(4) << j << std::endl;
	}
}
