#pragma once
#include "block/component.hpp"
#include "nlohmann/json_fwd.hpp"
#include "pool/package.hpp"
#include "util/placement.hpp"
#include "util/uuid.hpp"
#include "util/uuid_ptr.hpp"
#include <vector>

namespace horizon {
using json = nlohmann::json;

class BoardPackage {
public:
    BoardPackage(const UUID &uu, const json &, Block &block, class IPool &pool);
    BoardPackage(const UUID &uu, Component *comp);
    BoardPackage(const UUID &uu);
    BoardPackage(shallow_copy_t sh, const BoardPackage &other);
    UUID uuid;
    uuid_ptr<Component> component;
    std::shared_ptr<const Package> alternate_package;
    UUID model;

    std::shared_ptr<const Package> pool_package;
    Package package;
    bool update_package(const class Board &brd);
    void update_texts(const class Board &brd);
    void update_nets();
    void update(const class Board &brd);
    std::set<UUID> get_nets() const;

    Placement placement;
    std::pair<Coordi, Coordi> get_bbox() const;

    bool flip = false;
    bool smashed = false;
    bool omit_silkscreen = false;
    bool fixed = false;
    bool omit_outline = false;
    std::vector<uuid_ptr<Text>> texts;

    std::string replace_text(const std::string &t, bool *replaced = nullptr) const;

    UUID get_uuid() const;
    json serialize() const;
    static std::vector<UUID> peek_texts(const json &j);
};
} // namespace horizon
