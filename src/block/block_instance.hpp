#pragma once
#include "component.hpp"
#include "util/uuid_ptr.hpp"

namespace horizon {
using json = nlohmann::json;

class BlockInstanceMapping {
public:
    BlockInstanceMapping(const json &j);
    BlockInstanceMapping(const Block &block);

    class ComponentInfo {
    public:
        ComponentInfo(const json &j);
        ComponentInfo();

        std::string refdes;
        bool nopopulate = false;

        json serialize() const;
    };

    UUID block;
    std::map<UUID, ComponentInfo> components;
    json serialize() const;
};

class BlockInstance {
public:
    BlockInstance(const UUID &uu, const json &j, class Block &block, class IBlockProvider &prv);
    BlockInstance(const UUID &uu, Block &block);

    UUID uuid;

    uuid_ptr<Block> block;
    std::string refdes;

    std::map<UUID, Connection> connections;

    std::string replace_text(const std::string &t, bool *replaced = nullptr) const;

    UUID get_uuid() const;

    json serialize() const;
};
} // namespace horizon
