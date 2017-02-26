#include "core_symbol.hpp"
#include <algorithm>
#include <fstream>

namespace horizon {
	CoreSymbol::CoreSymbol(const std::string &filename, Pool &pool):
		sym(Symbol::new_from_file(filename, pool)),
		sym_work(sym),
		m_filename(filename)
	{
		rebuild();
		m_pool = &pool;
	}

	bool CoreSymbol::has_object_type(ObjectType ty) {
		switch(ty) {
			case ObjectType::JUNCTION:
			case ObjectType::LINE:
			case ObjectType::ARC:
			case ObjectType::SYMBOL_PIN:
			case ObjectType::TEXT:
				return true;
			break;
			default:
				;
		}

		return false;
	}

	std::map<UUID, Text> *CoreSymbol::get_text_map(bool work) {
		auto &s = work?sym_work:sym;
		return &s.texts;
	}


	Junction *CoreSymbol::get_junction(const UUID &uu, bool work) {
		auto &s = work?sym_work:sym;
		return s.get_junction(uu);
	}
	Line *CoreSymbol::get_line(const UUID &uu, bool work) {
		auto &s = work?sym_work:sym;
		return &s.lines.at(uu);
	}
	SymbolPin *CoreSymbol::get_symbol_pin(const UUID &uu, bool work) {
		auto &s = work?sym_work:sym;
		return s.get_symbol_pin(uu);
	}
	Arc *CoreSymbol::get_arc(const UUID &uu, bool work) {
		auto &s = work?sym_work:sym;
		return &s.arcs.at(uu);
	}

	Junction *CoreSymbol::insert_junction(const UUID &uu, bool work) {
		auto &s = work?sym_work:sym;
		auto x = s.junctions.emplace(std::make_pair(uu, uu));
		return &(x.first->second);
	}
	void CoreSymbol::delete_junction(const UUID &uu, bool work) {
		auto &s = work?sym_work:sym;
		s.junctions.erase(uu);
	}

	Line *CoreSymbol::insert_line(const UUID &uu, bool work) {
		auto &s = work?sym_work:sym;
		auto x = s.lines.emplace(std::make_pair(uu, uu));
		return &(x.first->second);
	}
	void CoreSymbol::delete_line(const UUID &uu, bool work) {
		auto &s = work?sym_work:sym;
		s.lines.erase(uu);
	}

	Arc *CoreSymbol::insert_arc(const UUID &uu, bool work) {
		auto &s = work?sym_work:sym;
		auto x = s.arcs.emplace(std::make_pair(uu, uu));
		return &(x.first->second);
	}
	void CoreSymbol::delete_arc(const UUID &uu, bool work) {
		auto &s = work?sym_work:sym;
		s.arcs.erase(uu);
	}

	void CoreSymbol::delete_symbol_pin(const UUID &uu, bool work) {
			auto &s = work?sym_work:sym;
			s.pins.erase(uu);
	}
	SymbolPin *CoreSymbol::insert_symbol_pin(const UUID &uu, bool work) {
			auto &s = work?sym_work:sym;
			auto x = s.pins.emplace(std::make_pair(uu, uu));
			return &(x.first->second);
	}

	std::vector<Line*> CoreSymbol::get_lines(bool work) {
		auto &s = work?sym_work:sym;
		std::vector<Line*> r;
		for(auto &it: s.lines) {
			r.push_back(&it.second);
		}
		return r;
	}

	std::vector<Arc*> CoreSymbol::get_arcs(bool work) {
		auto &s = work?sym_work:sym;
		std::vector<Arc*> r;
		for(auto &it: s.arcs) {
			r.push_back(&it.second);
		}
		return r;
	}

	std::vector<Pin*> CoreSymbol::get_pins(bool work) {
		auto &s = work?sym_work:sym;
		std::vector<Pin*> r;
		for(auto &it: s.unit->pins) {
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
		sym_work = sym;
		Core::rebuild(from_undo);
	}

	void CoreSymbol::history_push() {
		history.push_back(std::make_unique<CoreSymbol::HistoryItem>(sym));
	}

	void CoreSymbol::history_load(unsigned int i) {
		auto x = dynamic_cast<CoreSymbol::HistoryItem*>(history.at(history_current).get());
		sym = x->sym;
		rebuild(true);
	}

	const Symbol *CoreSymbol::get_canvas_data() {
		return &sym_work;
	}

	std::pair<Coordi,Coordi> CoreSymbol::get_bbox() {
		auto bb = sym_work.get_bbox(true);
		int64_t pad = 1_mm;
		bb.first.x -= pad;
		bb.first.y -= pad;

		bb.second.x += pad;
		bb.second.y += pad;
		return bb;
	}

	Symbol *CoreSymbol::get_symbol(bool work) {
		return work?&sym_work:&sym;
	}

	void CoreSymbol::commit() {
		sym = sym_work;
		set_needs_save(true);
	}

	void CoreSymbol::revert() {
		sym_work = sym;
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
