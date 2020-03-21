#pragma once
#include "block/component.hpp"
#include "nlohmann/json_fwd.hpp"
#include "pool/package.hpp"
#include "pool/pool.hpp"
#include "util/placement.hpp"
#include "util/uuid.hpp"
#include "util/uuid_provider.hpp"
#include "util/uuid_ptr.hpp"
#include <fstream>
#include <map>
#include <vector>

namespace horizon {
using json = nlohmann::json;

class BoardPackage : public UUIDProvider {
public:
    BoardPackage(const UUID &uu, const json &, Block &block, Pool &pool);
    BoardPackage(const UUID &uu, Component *comp);
    BoardPackage(const UUID &uu);
    BoardPackage(shallow_copy_t sh, const BoardPackage &other);
    UUID uuid;
    uuid_ptr<Component> component;
    const Package *alternate_package = nullptr;
    UUID model;

    const Package *pool_package;
    Package package;

    Placement placement;
    bool flip = false;
    bool smashed = false;
    bool omit_silkscreen = false;
    bool fixed = false;
    bool omit_outline = false;
    std::vector<uuid_ptr<Text>> texts;

    std::string replace_text(const std::string &t, bool *replaced = nullptr) const;

    UUID get_uuid() const override;
    json serialize() const;
    static std::vector<UUID> peek_texts(const json &j);
};
} // namespace horizon
