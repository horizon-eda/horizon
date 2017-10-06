#include "core_symbol.hpp"
#include <algorithm>
#include <fstream>

namespace horizon {
	CoreSymbol::CoreSymbol(const std::string &filename, Pool &pool):
		sym(Symbol::new_from_file(filename, pool)),
		m_filename(filename)
	{
		m_pool = &pool;
		rebuild();
	}

	bool CoreSymbol::has_object_type(ObjectType ty) {
		switch(ty) {
			case ObjectType::JUNCTION:
			case ObjectType::LINE:
			case ObjectType::ARC:
			case ObjectType::SYMBOL_PIN:
			case ObjectType::TEXT:
			case ObjectType::POLYGON:
			case ObjectType::POLYGON_EDGE:
			case ObjectType::POLYGON_VERTEX:
				return true;
			break;
			default:
				;
		}

		return false;
	}

	std::map<UUID, Text> *CoreSymbol::get_text_map(bool work) {
		return &sym.texts;
	}

	std::map<UUID, Polygon> *CoreSymbol::get_polygon_map(bool work) {
		return &sym.polygons;
	}


	Junction *CoreSymbol::get_junction(const UUID &uu, bool work) {
		return sym.get_junction(uu);
	}
	Line *CoreSymbol::get_line(const UUID &uu, bool work) {
		return &sym.lines.at(uu);
	}
	SymbolPin *CoreSymbol::get_symbol_pin(const UUID &uu, bool work) {
		return sym.get_symbol_pin(uu);
	}
	Arc *CoreSymbol::get_arc(const UUID &uu, bool work) {
		return &sym.arcs.at(uu);
	}

	Junction *CoreSymbol::insert_junction(const UUID &uu, bool work) {
		auto x = sym.junctions.emplace(std::make_pair(uu, uu));
		return &(x.first->second);
	}
	void CoreSymbol::delete_junction(const UUID &uu, bool work) {
		sym.junctions.erase(uu);
	}

	Line *CoreSymbol::insert_line(const UUID &uu, bool work) {
		auto x = sym.lines.emplace(std::make_pair(uu, uu));
		return &(x.first->second);
	}
	void CoreSymbol::delete_line(const UUID &uu, bool work) {
		sym.lines.erase(uu);
	}

	Arc *CoreSymbol::insert_arc(const UUID &uu, bool work) {
		auto x = sym.arcs.emplace(std::make_pair(uu, uu));
		return &(x.first->second);
	}
	void CoreSymbol::delete_arc(const UUID &uu, bool work) {
		sym.arcs.erase(uu);
	}

	void CoreSymbol::delete_symbol_pin(const UUID &uu, bool work) {
			sym.pins.erase(uu);
	}
	SymbolPin *CoreSymbol::insert_symbol_pin(const UUID &uu, bool work) {
			auto x = sym.pins.emplace(std::make_pair(uu, uu));
			return &(x.first->second);
	}

	std::vector<Line*> CoreSymbol::get_lines(bool work) {
		std::vector<Line*> r;
		for(auto &it: sym.lines) {
			r.push_back(&it.second);
		}
		return r;
	}

	std::vector<Arc*> CoreSymbol::get_arcs(bool work) {
		std::vector<Arc*> r;
		for(auto &it: sym.arcs) {
			r.push_back(&it.second);
		}
		return r;
	}

	std::vector<const Pin*> CoreSymbol::get_pins(bool work) {
		std::vector<const Pin*> r;
		for(auto &it: sym.unit->pins) {
			r.push_back(&it.second);
		}
		return r;
	}

	std::string CoreSymbol::get_property_string(const UUID &uu, ObjectType type, ObjectProperty::ID property, bool *handled) {
		bool h = false;
		std::string r= Core::get_property_string(uu, type, property, &h);
		if(h)
			return r;
		switch(type) {
			case ObjectType::SYMBOL_PIN :
				switch(property) {
					case ObjectProperty::ID::NAME :
						return sym.unit->pins.at(uu).primary_name;
					default :
						return "<invalid prop>";
				}
			break;

			default :
				return "<invalid type>";
		}
		return "<meh>";
	}

	bool CoreSymbol::get_property_bool(const UUID &uu, ObjectType type, ObjectProperty::ID property, bool *handled) {
		bool h = false;
		bool r= Core::get_property_bool(uu, type, property, &h);
		if(h)
			return r;

		switch(type) {
			case ObjectType::SYMBOL_PIN :
				switch(property) {
					case ObjectProperty::ID::NAME_VISIBLE :
						return sym.pins.at(uu).name_visible;
					case ObjectProperty::ID::PAD_VISIBLE :
						return sym.pins.at(uu).pad_visible;
					default :
						return false;
				}
			break;

			default :
				return false;
		}
	}


	void CoreSymbol::set_property_bool(const UUID &uu, ObjectType type, ObjectProperty::ID property, bool value, bool *handled) {
		if(tool_is_active())
			return;
		bool h = false;
		Core::set_property_bool(uu, type, property, value, &h);
		if(h)
			return;
		switch(type) {
			case ObjectType::SYMBOL_PIN :
				switch(property) {
					case ObjectProperty::ID::NAME_VISIBLE :
						sym.pins.at(uu).name_visible = value;
					break;
					case ObjectProperty::ID::PAD_VISIBLE :
						sym.pins.at(uu).pad_visible = value;
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
	int64_t CoreSymbol::get_property_int(const UUID &uu, ObjectType type, ObjectProperty::ID property, bool *handled) {
		bool h = false;
		int64_t r= Core::get_property_int(uu, type, property, &h);
		if(h)
			return r;
		switch(type) {
			case ObjectType::SYMBOL_PIN :
				switch(property) {
					case ObjectProperty::ID::LENGTH:
						return sym.pins.at(uu).length;
					default :
						return 0;
				}
			break;

			default :
				return 0;
		}
	}
	void CoreSymbol::set_property_int(const UUID &uu, ObjectType type, ObjectProperty::ID property, int64_t value, bool *handled) {
		if(tool_is_active())
			return;
		bool h = false;
		Core::set_property_int(uu, type, property, value, &h);
		if(h)
			return;
		switch(type) {
			case ObjectType::SYMBOL_PIN :
				switch(property) {
					case ObjectProperty::ID::LENGTH:
						sym.pins.at(uu).length = std::max(value, (int64_t)0);
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

	void CoreSymbol::rebuild(bool from_undo) {
		sym.expand();
		Core::rebuild(from_undo);
	}

	void CoreSymbol::history_push() {
		history.push_back(std::make_unique<CoreSymbol::HistoryItem>(sym));
	}

	void CoreSymbol::history_load(unsigned int i) {
		auto x = dynamic_cast<CoreSymbol::HistoryItem*>(history.at(history_current).get());
		sym = x->sym;
		s_signal_rebuilt.emit();
	}

	LayerProvider *CoreSymbol::get_layer_provider() {
		return &sym;
	}

	const Symbol *CoreSymbol::get_canvas_data() {
		return &sym;
	}

	std::pair<Coordi,Coordi> CoreSymbol::get_bbox() {
		auto bb = sym.get_bbox(true);
		int64_t pad = 1_mm;
		bb.first.x -= pad;
		bb.first.y -= pad;

		bb.second.x += pad;
		bb.second.y += pad;
		return bb;
	}

	Symbol *CoreSymbol::get_symbol(bool work) {
		return &sym;
	}

	void CoreSymbol::commit() {
		set_needs_save(true);
	}

	void CoreSymbol::revert() {
		history_load(history_current);
		reverted = true;
	}

	void CoreSymbol::save() {
		s_signal_save.emit();

		std::ofstream ofs(m_filename);
		if(!ofs.is_open()) {
			std::cout << "can't save symbol" <<std::endl;
			return;
		}
		json j = sym.serialize();
		ofs << std::setw(4) << j;
		ofs.close();
		set_needs_save(false);
	}
}
