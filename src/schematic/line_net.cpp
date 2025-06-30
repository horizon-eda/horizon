#include "line_net.hpp"
#include "common/lut.hpp"
#include "schematic_symbol.hpp"
#include "sheet.hpp"
#include <nlohmann/json.hpp>

namespace horizon {

LineNet::Connection::Connection(const json &j, Sheet *sheet)
{
    if (!j.at("junc").is_null()) {
        if (sheet)
            junc = &sheet->junctions.at(j.at("junc").get<std::string>());
        else
            junc.uuid = j.at("junc").get<std::string>();
    }
    else if (!j.at("pin").is_null()) {
        UUIDPath<2> pin_path(j.at("pin").get<std::string>());
        if (sheet) {
            symbol = &sheet->symbols.at(pin_path.at(0));
            pin = &symbol->symbol.pins.at(pin_path.at(1));
        }
        else {
            symbol.uuid = pin_path.at(0);
            pin.uuid = pin_path.at(1);
        }
    }
    else if (j.count("port") && !j.at("port").is_null()) {
        UUIDPath<2> port_path(j.at("port").get<std::string>());
        if (sheet) {
            block_symbol = &sheet->block_symbols.at(port_path.at(0));
            port = &block_symbol->symbol.ports.at(port_path.at(1));
        }
        else {
            block_symbol.uuid = port_path.at(0);
            port.uuid = port_path.at(1);
        }
    }
    else if (!j.at("bus_ripper").is_null()) {
        if (sheet)
            bus_ripper = &sheet->bus_rippers.at(j.at("bus_ripper").get<std::string>());
        else
            bus_ripper.uuid = j.at("bus_ripper").get<std::string>();
    }
    else {
        assert(false);
    }
}

UUIDPath<2> LineNet::Connection::get_pin_path() const
{
    assert(is_pin());
    return UUIDPath<2>(symbol->uuid, pin->uuid);
}


UUIDPath<2> LineNet::Connection::get_port_path() const
{
    assert(is_port());
    return UUIDPath<2>(block_symbol->uuid, port->uuid);
}

bool LineNet::Connection::is_junc() const
{
    if (junc) {
#ifdef CONNECTION_CHECK
        assert(!is_pin());
        assert(!is_bus_ripper());
        assert(!is_port());
#endif
        return true;
    }
    return false;
}
bool LineNet::Connection::is_bus_ripper() const
{
    if (bus_ripper) {
#ifdef CONNECTION_CHECK
        assert(!is_pin());
        assert(!is_junc());
        assert(!is_port());
#endif
        return true;
    }
    return false;
}

bool LineNet::Connection::is_pin() const
{
    if (symbol) {
#ifdef CONNECTION_CHECK
        assert(pin);
        assert(!is_junc());
        assert(!is_bus_ripper());
        assert(!is_port());
#endif
        return true;
    }
    return false;
}

bool LineNet::Connection::is_port() const
{
    if (block_symbol) {
#ifdef CONNECTION_CHECK
        assert(port);
        assert(!is_junc());
        assert(!is_bus_ripper());
        assert(!is_pin());
#endif
        return true;
    }
    return false;
}

void LineNet::Connection::connect(SchematicJunction *j)
{
    junc = j;
    symbol = nullptr;
    pin = nullptr;
    bus_ripper = nullptr;
    block_symbol = nullptr;
    port = nullptr;
}

void LineNet::Connection::connect(SchematicSymbol *s, SymbolPin *p)
{
    junc = nullptr;
    symbol = s;
    pin = p;
    bus_ripper = nullptr;
    block_symbol = nullptr;
    port = nullptr;
}

void LineNet::Connection::connect(SchematicBlockSymbol *s, BlockSymbolPort *p)
{
    junc = nullptr;
    symbol = nullptr;
    pin = nullptr;
    bus_ripper = nullptr;
    block_symbol = s;
    port = p;
}

void LineNet::Connection::connect(BusRipper *r)
{
    junc = nullptr;
    symbol = nullptr;
    pin = nullptr;
    bus_ripper = r;
    block_symbol = nullptr;
    port = nullptr;
}

void LineNet::Connection::update_refs(Sheet &sheet)
{
    junc.update(sheet.junctions);
    symbol.update(sheet.symbols);
    if (symbol)
        pin.update(symbol->symbol.pins);
    bus_ripper.update(sheet.bus_rippers);
    block_symbol.update(sheet.block_symbols);
    if (block_symbol)
        port.update(block_symbol->symbol.ports);
}

Coordi LineNet::Connection::get_position() const
{
    if (is_junc()) {
        return junc->position;
    }
    else if (is_pin()) {
        return symbol->placement.transform(pin->position);
    }
    else if (is_bus_ripper()) {
        return bus_ripper->get_connector_pos();
    }
    else if (is_port()) {
        return block_symbol->placement.transform(port->position);
    }
    else {
        assert(false);
    }
}

json LineNet::Connection::serialize() const
{
    json j;
    j["junc"] = nullptr;
    j["pin"] = nullptr;
    j["bus_ripper"] = nullptr;
    j["port"] = nullptr;
    if (is_junc()) {
        j["junc"] = (std::string)junc->uuid;
    }
    else if (is_pin()) {
        j["pin"] = (std::string)get_pin_path();
    }
    else if (is_port()) {
        j["port"] = (std::string)get_port_path();
    }
    else if (is_bus_ripper()) {
        j["bus_ripper"] = (std::string)bus_ripper->uuid;
    }
    else {
        assert(false);
    }
    return j;
}

UUID LineNet::Connection::get_net_segment() const
{
    if (is_junc()) {
        return junc->net_segment;
    }
    else if (is_pin()) {
        return pin->net_segment;
    }
    else if (is_bus_ripper()) {
        return bus_ripper->net_segment;
    }
    else if (is_port()) {
        return port->net_segment;
    }
    else {
        assert(false);
        return UUID();
    }
}

bool LineNet::Connection::operator<(const LineNet::Connection &other) const
{
    if (junc < other.junc)
        return true;
    if (junc > other.junc)
        return false;
    if (bus_ripper < other.bus_ripper)
        return true;
    if (bus_ripper > other.bus_ripper)
        return false;
    if (port < other.port)
        return true;
    if (port > other.port)
        return false;
    return pin < other.pin;
}


LineNet::LineNet(const UUID &uu, const json &j, Sheet *sheet)
    : uuid(uu), from(j.at("from"), sheet), to(j.at("to"), sheet)
{
}

void LineNet::update_refs(Sheet &sheet)
{
    to.update_refs(sheet);
    from.update_refs(sheet);
}

bool LineNet::is_connected_to_symbol(const UUID &uu_sym, const UUID &uu_pin) const
{
    for (auto &it : {to, from}) {
        if (it.symbol && (it.symbol->uuid == uu_sym) && (it.pin->uuid == uu_pin))
            return true;
    }
    return false;
}

bool LineNet::is_connected_to_block_symbol(const UUID &uu_sym, const UUID &uu_port) const
{
    for (auto &it : {to, from}) {
        if (it.block_symbol && (it.block_symbol->uuid == uu_sym) && (it.port->uuid == uu_port))
            return true;
    }
    return false;
}

UUID LineNet::get_uuid() const
{
    return uuid;
}

LineNet::LineNet(const UUID &uu) : uuid(uu)
{
}

json LineNet::serialize() const
{
    json j;
    j["from"] = from.serialize();
    j["to"] = to.serialize();
    return j;
}

bool LineNet::coord_on_line(const Coordi &p) const
{
    Coordi a = Coordi::min(from.get_position(), to.get_position());
    Coordi b = Coordi::max(from.get_position(), to.get_position());
    if (from.get_position() == p || to.get_position() == p)
        return false;
    if (p.x >= a.x && p.x <= b.x && p.y >= a.y && p.y <= b.y) { // inside bbox
        auto c = to.get_position() - from.get_position();
        auto d = p - from.get_position();
        if ((c.dot(d)) * (c.dot(d)) == c.mag_sq() * d.mag_sq()) {
            return true;
        }
    }
    return false;
}
} // namespace horizon
