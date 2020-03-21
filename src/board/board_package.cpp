#include "board_package.hpp"
#include "pool/part.hpp"
#include "nlohmann/json.hpp"
#include "block/block.hpp"

namespace horizon {

BoardPackage::BoardPackage(const UUID &uu, const json &j, Block &block, Pool &pool)
    : uuid(uu), component(&block.components.at(j.at("component").get<std::string>())),
      pool_package(component->part->package), package(*pool_package), placement(j.at("placement")),
      flip(j.at("flip").get<bool>()), smashed(j.value("smashed", false)),
      omit_silkscreen(j.value("omit_silkscreen", false)), fixed(j.value("fixed", false)),
      omit_outline(j.value("omit_outline", false))
{
    if (j.count("texts")) {
        const json &o = j.at("texts");
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            texts.emplace_back(UUID(it.value().get<std::string>()));
        }
    }
    if (j.count("alternate_package")) {
        alternate_package = pool.get_package(j.at("alternate_package").get<std::string>());
    }
}
BoardPackage::BoardPackage(const UUID &uu, Component *comp)
    : uuid(uu), component(comp), pool_package(component->part->package), package(*pool_package)
{
}

BoardPackage::BoardPackage(const UUID &uu) : uuid(uu), component(nullptr), pool_package(nullptr), package(UUID())
{
}

json BoardPackage::serialize() const
{
    json j;
    j["component"] = (std::string)component.uuid;
    j["placement"] = placement.serialize();
    j["flip"] = flip;
    j["smashed"] = smashed;
    j["omit_silkscreen"] = omit_silkscreen;
    if (omit_outline)
        j["omit_outline"] = true;
    j["fixed"] = fixed;
    j["texts"] = json::array();
    for (const auto &it : texts) {
        j["texts"].push_back((std::string)it->uuid);
    }
    if (alternate_package)
        j["alternate_package"] = (std::string)alternate_package->uuid;

    return j;
}

std::string BoardPackage::replace_text(const std::string &t, bool *replaced) const
{
    return component->replace_text(t, replaced);
}

UUID BoardPackage::get_uuid() const
{
    return uuid;
}
BoardPackage::BoardPackage(shallow_copy_t sh, const BoardPackage &other)
    : uuid(other.uuid), component(other.component), alternate_package(other.alternate_package), model(other.model),
      pool_package(other.pool_package), package(other.package.uuid), placement(other.placement), flip(other.flip),
      smashed(other.smashed), omit_silkscreen(other.omit_silkscreen), fixed(other.fixed),
      omit_outline(other.omit_outline), texts(other.texts)
{
}

std::vector<UUID> BoardPackage::peek_texts(const json &j)
{
    std::vector<UUID> r;
    if (j.count("texts")) {
        const json &o = j.at("texts");
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            r.emplace_back(UUID(it.value().get<std::string>()));
        }
    }
    return r;
}

} // namespace horizon
