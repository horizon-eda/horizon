#pragma once
#include "common/common.hpp"
#include "nlohmann/json_fwd.hpp"
#include "net.hpp"
#include "util/uuid.hpp"
#include "util/uuid_path.hpp"
#include "util/uuid_ptr.hpp"
#include "util/uuid_vec.hpp"
#include <map>
#include <set>

namespace horizon {
using json = nlohmann::json;

class Connection {
public:
    Connection(const json &j, class Block *block);
    Connection(Net *n) : net(n)
    {
    }
    uuid_ptr<Net> net;

    json serialize() const;
};

/**
 * A Component is an instanced Entity in a Block.
 * Like in other places around horizon, a Component is identified
 * by its UUID, not by its reference designator. Ensuring unique refdes
 * is up to other parts of the application.
 *
 * Usually, a Component will be assigned a Part to map it to a real-world
 * thing one can order and put on a PCB. The Part must have the same Entity
 * as the Component. The assignment can be changed at any time. This simplifies
 * exchanging parts for logically identical parts of the same kind.
 * When a part Pis assigned, the value of the Component gets overriden by the
 * Part.
 */
class Component {
public:
    Component(const UUID &uu, const json &j, class IPool &pool, class Block *block = nullptr);
    Component(const UUID &uu);

    UUID get_uuid() const;

    UUID uuid;
    const class Entity *entity = nullptr;
    const class Part *part = nullptr;
    std::string refdes;
    std::string value;
    UUID group;
    UUID tag;
    bool nopopulate = false;

    UUIDVec href;

    enum class NopopulateFromInstance { SET, CLEAR, UNSET };
    NopopulateFromInstance nopopulate_from_instance = NopopulateFromInstance::UNSET;

    bool get_nopopulate() const;

    const std::string &get_prefix() const;

    /**
     * which Nins are connected to which Net
     * the UUIDPath consists of Gate and Pin UUID
     */
    std::map<UUIDPath<2>, Connection> connections;

    /**
     * used to select alternate pin names
     */
    class AltPinInfo {
    public:
        AltPinInfo() = default;
        AltPinInfo(const json &j, const Pin &pin);
        std::set<UUID> pin_names;
        bool use_primary_name = false;
        bool use_custom_name = false;

        std::string custom_name;
        Pin::Direction custom_direction = Pin::Direction::BIDIRECTIONAL;

        void update_for_index(int index, const Pin &pin);

        json serialize() const;
    };
    std::map<UUIDPath<2>, AltPinInfo> alt_pins;

    static Pin::Direction get_effective_direction(const Component::AltPinInfo &alt, const Pin &pin);
    Pin::Direction get_effective_direction(const UUIDPath<2> &path) const;

    std::string replace_text(const std::string &t, bool *replaced = nullptr) const;

    json serialize() const;
    virtual ~Component()
    {
    }

private:
    // void update_refs();
};
} // namespace horizon
