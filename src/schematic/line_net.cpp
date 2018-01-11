#include "line_net.hpp"
#include "common/lut.hpp"
#include "schematic_symbol.hpp"
#include "sheet.hpp"

namespace horizon {

	LineNet::Connection::Connection(const json &j, Sheet *sheet) {
		if(!j.at("junc").is_null()) {
			if(sheet)
				junc = &sheet->junctions.at(j.at("junc").get<std::string>());
			else
				junc.uuid = j.at("junc").get<std::string>();
		}
		else if(!j.at("pin").is_null()) {
			UUIDPath<2> pin_path(j.at("pin").get<std::string>());
			if(sheet) {
				symbol = &sheet->symbols.at(pin_path.at(0));
				pin = &symbol->symbol.pins.at(pin_path.at(1));
			}
			else {
				symbol.uuid = pin_path.at(0);
				pin.uuid = pin_path.at(1);
			}
		}
		else if(!j.at("bus_ripper").is_null()) {
			if(sheet)
				bus_ripper = &sheet->bus_rippers.at(j.at("bus_ripper").get<std::string>());
			else
				bus_ripper.uuid = j.at("bus_ripper").get<std::string>();
		}
		else {
			assert(false);
		}
	}

	UUIDPath<2 >LineNet::Connection::get_pin_path() const {
		assert(junc==nullptr);
		return UUIDPath<2>(symbol->uuid, pin->uuid);
	}

	bool LineNet::Connection::is_junc() const {
		if(junc) {
			assert(!is_pin());
			assert(!is_bus_ripper());
			return true;
		}
		return false;
	}
	bool LineNet::Connection::is_bus_ripper() const {
		if(bus_ripper) {
			assert(!is_pin());
			assert(!is_junc());
			return true;
		}
		return false;
	}

	bool LineNet::Connection::is_pin() const {
		if(symbol) {
			assert(pin);
			assert(!is_junc());
			assert(!is_bus_ripper());
			return true;
		}
		return false;
	}

	void LineNet::Connection::connect(Junction *j) {
		junc = j;
		symbol = nullptr;
		pin = nullptr;
		bus_ripper = nullptr;
	}

	void LineNet::Connection::connect(SchematicSymbol *s, SymbolPin *p) {
		junc = nullptr;
		symbol = s;
		pin = p;
		bus_ripper = nullptr;
	}
	void LineNet::Connection::connect(BusRipper *r) {
		junc = nullptr;;
		symbol = nullptr;
		pin = nullptr;
		bus_ripper = r;
	}

	void LineNet::Connection::update_refs(Sheet &sheet) {
		junc.update(sheet.junctions);
		symbol.update(sheet.symbols);
		if(symbol)
			pin.update(symbol->symbol.pins);
		bus_ripper.update(sheet.bus_rippers);
	}

	Coordi LineNet::Connection::get_position() const {
		if(is_junc()) {
			return junc->position;
		}
		else if(is_pin()) {
			return symbol->placement.transform(pin->position);
		}
		else if(is_bus_ripper()) {
			return bus_ripper->get_connector_pos();
		}
		else {
			assert(false);
		}
	}

	json LineNet::Connection::serialize() const {
		json j;
		j["junc"] = nullptr;
		j["pin"] = nullptr;
		j["bus_ripper"] = nullptr;
		if(is_junc()) {
			j["junc"] = (std::string)junc->uuid;
		}
		else if(is_pin()) {
			j["pin"] = (std::string)get_pin_path();
		}
		else if(is_bus_ripper()) {
			j["bus_ripper"] = (std::string)bus_ripper->uuid;
		}
		else {
			assert(false);
		}
		return j;
	}

	UUID LineNet::Connection::get_net_segment() const {
		if(is_junc()) {
			return junc->net_segment;
		}
		else if(is_pin()) {
			return pin->net_segment;
		}
		else if(is_bus_ripper()) {
			return bus_ripper->net_segment;
		}
		else {
			assert(false);
			return UUID();
		}
	}

	bool LineNet::Connection::operator <(const LineNet::Connection &other) const {
		if(junc < other.junc)
			return true;
		if(junc > other.junc)
			return false;
		if(bus_ripper < other.bus_ripper)
			return true;
		if(bus_ripper > other.bus_ripper)
			return false;
		return pin < other.pin;
	}

	
	LineNet::LineNet(const UUID &uu, const json &j, Sheet *sheet):
		uuid(uu),
		from(j["from"], sheet),
		to(j["to"], sheet)
	{

	}
	
	void LineNet::update_refs(Sheet &sheet) {
		to.update_refs(sheet);
		from.update_refs(sheet);
	}

	bool LineNet::is_connected_to(const UUID &uu_sym, const UUID &uu_pin) const {
		for(auto &it: {to, from}) {
			if(it.symbol && (it.symbol->uuid == uu_sym) && (it.pin->uuid == uu_pin))
				return true;
		}
		return false;
	}

	UUID LineNet::get_uuid() const {
		return uuid;
	}

	LineNet::LineNet(const UUID &uu): uuid(uu) {}

	json LineNet::serialize() const {
		json j;
		j["from"] = from.serialize();
		j["to"] = to.serialize();
		return j;
	}

	bool LineNet::coord_on_line(const Coordi &p) const {
		Coordi a = Coordi::min(from.get_position(), to.get_position());
		Coordi b = Coordi::max(from.get_position(), to.get_position());
		if(from.get_position() == p || to.get_position() == p)
			return false;
		if(p.x >= a.x && p.x <= b.x && p.y >= a.y && p.y <= b.y) { //inside bbox
			auto c = to.get_position()-from.get_position();
			auto d = p-from.get_position();
			if((c.dot(d))*(c.dot(d)) == c.mag_sq()*d.mag_sq()) {
				return true;
			}
		}
		return false;
	}
}
