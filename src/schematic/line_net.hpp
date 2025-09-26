#pragma once
#include "util/uuid.hpp"
#include <nlohmann/json_fwd.hpp>
#include "common/common.hpp"
#include "util/uuid_ptr.hpp"
#include "util/uuid_path.hpp"
#include <vector>
#include <map>

namespace horizon {
using json = nlohmann::json;

/**
 * LineNet is similar to Line, except it denotes electrical connection.
 * When connected to a BusLabel, it denotes a Bus.
 */
class LineNet {
public:
    enum class End { TO, FROM };

    LineNet(const UUID &uu, const json &j, class Sheet *sheet = nullptr);
    LineNet(const UUID &uu);

    void update_refs(class Sheet &sheet);
    bool is_connected_to_symbol(const UUID &uu_sym, const UUID &uu_pin) const;
    bool is_connected_to_block_symbol(const UUID &uu_sym, const UUID &uu_port) const;
    UUID get_uuid() const;
    bool coord_on_line(const Coordi &coord) const;

    uuid_ptr<class Net> net = nullptr;
    uuid_ptr<class Bus> bus = nullptr;
    UUID net_segment = UUID();


    UUID uuid;

    class Connection {
    public:
        Connection()
        {
        }
        Connection(const json &j, Sheet *sheet);
        uuid_ptr<class SchematicJunction> junc = nullptr;
        uuid_ptr<class SchematicSymbol> symbol = nullptr;
        uuid_ptr<class SymbolPin> pin = nullptr;
        uuid_ptr<class BusRipper> bus_ripper = nullptr;
        uuid_ptr<class SchematicBlockSymbol> block_symbol = nullptr;
        uuid_ptr<class BlockSymbolPort> port = nullptr;
        bool operator<(const Connection &other) const;
        bool operator==(const Connection &other) const;

        void connect(SchematicJunction *j);
        void connect(BusRipper *r);
        void connect(SchematicSymbol *j, SymbolPin *pin);
        void connect(SchematicBlockSymbol *j, BlockSymbolPort *port);
        UUIDPath<2> get_pin_path() const;
        UUIDPath<2> get_port_path() const;
        bool is_junc() const;
        bool is_pin() const;
        bool is_bus_ripper() const;
        bool is_port() const;
        UUID get_net_segment() const;
        void update_refs(class Sheet &sheet);
        Coordi get_position() const;
        json serialize() const;
    };

    Connection from;
    Connection to;


    json serialize() const;
};
} // namespace horizon
