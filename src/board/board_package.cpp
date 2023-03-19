#include "board_package.hpp"
#include "pool/part.hpp"
#include "nlohmann/json.hpp"
#include "block/block.hpp"
#include "pool/ipool.hpp"
#include "board.hpp"
#include "util/util.hpp"
#include "board_layers.hpp"

namespace horizon {

BoardPackage::BoardPackage(const UUID &uu, const json &j, Block &block, IPool &pool)
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

bool BoardPackage::update_package(const Board &brd)
{
    bool ret = true;
    pool_package = component->part->package;
    model = component->part->get_model();

    if (alternate_package) {
        std::set<std::string> pads_from_primary, pads_from_alt;
        for (const auto &it_pad : pool_package->pads) {
            if (it_pad.second.padstack.type != Padstack::Type::MECHANICAL)
                pads_from_primary.insert(it_pad.second.name);
        }
        bool alt_valid = true;
        for (const auto &it_pad : alternate_package->pads) {
            if (it_pad.second.padstack.type != Padstack::Type::MECHANICAL) {
                if (!pads_from_alt.insert(it_pad.second.name).second) { // duplicate pad name
                    alt_valid = false;
                }
            }
        }
        if (!alt_valid || pads_from_alt != pads_from_primary) { // alt pkg isn't pad-equal
            package = *pool_package;
            ret = false;
        }
        else {
            package = *alternate_package;

            // need to adjust pad uuids to primary package
            map_erase_if(package.pads,
                         [](const auto &x) { return x.second.padstack.type != Padstack::Type::MECHANICAL; });
            std::map<std::string, UUID> pad_uuids;
            for (const auto &it_pad : pool_package->pads) {
                if (it_pad.second.padstack.type != Padstack::Type::MECHANICAL)
                    assert(pad_uuids.emplace(it_pad.second.name, it_pad.first).second); // no duplicates
            }
            for (const auto &it_pad : alternate_package->pads) {
                if (it_pad.second.padstack.type != Padstack::Type::MECHANICAL) {
                    auto uu = pad_uuids.at(it_pad.second.name);
                    auto &pad = package.pads.emplace(uu, it_pad.second).first->second;
                    pad.uuid = uu;
                }
            }

            if (package.models.size() == 1) { // alt pkg only has one model
                model = package.models.begin()->first;
            }
        }
    }
    else {
        package = *pool_package;
    }

    placement.mirror = flip;
    for (auto &it2 : package.pads) {
        it2.second.padstack.expand_inner(brd.get_n_inner_layers(), BoardLayers::layer_range_through);
    }

    if (flip) {
        for (auto &it2 : package.lines) {
            brd.flip_package_layer(it2.second.layer);
        }
        for (auto &it2 : package.arcs) {
            brd.flip_package_layer(it2.second.layer);
        }
        for (auto &it2 : package.texts) {
            brd.flip_package_layer(it2.second.layer);
        }
        for (auto &it2 : package.polygons) {
            brd.flip_package_layer(it2.second.layer);
        }
        for (auto &it2 : package.pads) {
            if (it2.second.padstack.type == Padstack::Type::TOP) {
                it2.second.padstack.type = Padstack::Type::BOTTOM;
            }
            else if (it2.second.padstack.type == Padstack::Type::BOTTOM) {
                it2.second.padstack.type = Padstack::Type::TOP;
            }
            for (auto &it3 : it2.second.padstack.polygons) {
                brd.flip_package_layer(it3.second.layer);
            }
            for (auto &it3 : it2.second.padstack.shapes) {
                brd.flip_package_layer(it3.second.layer);
            }
        }
    }
    return ret;
}

void BoardPackage::update_texts(const class Board &brd)
{
    map_erase_if(texts, [&brd](const auto &x) { return brd.texts.count(x.uuid) == 0; });

    for (auto &it_text : package.texts) {
        it_text.second.text = replace_text(it_text.second.text);
    }
    for (auto it_text : texts) {
        it_text->text_override = replace_text(it_text->text, &it_text->overridden);
    }
}

void BoardPackage::update_nets()
{
    for (auto &it_pad : package.pads) {
        it_pad.second.is_nc = false;
        if (component->part->pad_map.count(it_pad.first)) {
            const auto &pad_map_item = component->part->pad_map.at(it_pad.first);
            auto pin_path = UUIDPath<2>(pad_map_item.gate->uuid, pad_map_item.pin->uuid);
            if (component->connections.count(pin_path)) {
                const auto &conn = component->connections.at(pin_path);
                it_pad.second.net = conn.net;
                if (conn.net) {
                    it_pad.second.secondary_text = conn.net->name;
                }
                else {
                    it_pad.second.secondary_text = "NC";
                    it_pad.second.is_nc = true;
                }
            }
            else {
                it_pad.second.net = nullptr;
                it_pad.second.secondary_text =
                        "("
                        + component->part->entity->gates.at(pin_path.at(0)).unit->pins.at(pin_path.at(1)).primary_name
                        + ")";
            }
        }
        else {
            it_pad.second.net = nullptr;
            it_pad.second.secondary_text = "NC";
            it_pad.second.is_nc = true;
        }
    }
}

void BoardPackage::update(const Board &brd)
{
    update_package(brd);
    package.apply_parameter_set(brd.get_parameters());
    update_texts(brd);
    update_nets();
}

std::set<UUID> BoardPackage::get_nets() const
{
    std::set<UUID> nets;
    for (const auto &[uu, pad] : package.pads) {
        if (pad.net)
            nets.insert(pad.net->uuid);
    }
    return nets;
}


std::pair<Coordi, Coordi> BoardPackage::get_bbox() const
{
    return placement.transform_bb(package.get_bbox());
}

} // namespace horizon
