#include "part.hpp"
#include "ipool.hpp"
#include "entity.hpp"
#include "package.hpp"
#include "common/lut.hpp"
#include "nlohmann/json.hpp"
#include "util/util.hpp"
#include "pool/pool_parametric.hpp"
#include "logger/logger.hpp"

namespace horizon {

static const unsigned int app_version = 2;

unsigned int Part::get_app_version()
{
    return app_version;
}

static const LutEnumStr<Part::Flag> flag_lut = {
        {"base_part", Part::Flag::BASE_PART},
        {"exclude_bom", Part::Flag::EXCLUDE_BOM},
        {"exclude_pnp", Part::Flag::EXCLUDE_PNP},
};

static const LutEnumStr<Part::FlagState> flag_state_lut = {
        {"set", Part::FlagState::SET},
        {"clear", Part::FlagState::CLEAR},
        {"inherit", Part::FlagState::INHERIT},
};

static const LutEnumStr<Part::OverridePrefix> override_prefix_lut = {
        {"no", Part::OverridePrefix::NO},
        {"yes", Part::OverridePrefix::YES},
        {"inherit", Part::OverridePrefix::INHERIT},
};

void init_flags(decltype(Part::flags) &flags)
{
    flags.emplace(Part::Flag::BASE_PART, Part::FlagState::CLEAR);
    flags.emplace(Part::Flag::EXCLUDE_BOM, Part::FlagState::CLEAR);
    flags.emplace(Part::Flag::EXCLUDE_PNP, Part::FlagState::CLEAR);
}

Part::Part(const UUID &uu, const json &j, IPool &pool)
    : uuid(uu), inherit_tags(j.value("inherit_tags", false)), inherit_model(j.value("inherit_model", true)),
      version(app_version, j)
{
    check_object_type(j, ObjectType::PART);
    attributes[Attribute::MPN] = {j.at("MPN").at(0).get<bool>(), j.at("MPN").at(1).get<std::string>()};
    attributes[Attribute::VALUE] = {j.at("value").at(0).get<bool>(), j.at("value").at(1).get<std::string>()};
    attributes[Attribute::MANUFACTURER] = {j.at("manufacturer").at(0).get<bool>(),
                                           j.at("manufacturer").at(1).get<std::string>()};
    if (j.count("datasheet"))
        attributes[Attribute::DATASHEET] = {j.at("datasheet").at(0).get<bool>(),
                                            j.at("datasheet").at(1).get<std::string>()};
    else
        attributes[Attribute::DATASHEET] = {false, ""};

    if (j.count("description"))
        attributes[Attribute::DESCRIPTION] = {j.at("description").at(0).get<bool>(),
                                              j.at("description").at(1).get<std::string>()};
    else
        attributes[Attribute::DESCRIPTION] = {false, ""};

    if (j.count("model"))
        model = j.at("model").get<std::string>();

    if (j.count("base")) {
        base = pool.get_part(j.at("base").get<std::string>());
        entity = base->entity;
        package = base->package;
        pad_map = base->pad_map;
        /*if(MPN_raw == "$") {
                MPN = base->MPN;
        }
        if(value_raw == "$") {
                value = base->value;
        }
        if(manufacturer_raw == "$") {
                manufacturer = base->manufacturer;
        }*/
    }
    else {
        entity = pool.get_entity(j.at("entity").get<std::string>());
        package = pool.get_package(j.at("package").get<std::string>());
        const json &o = j.at("pad_map");
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto pad_uuid = UUID(it.key());
            if (package->pads.count(pad_uuid)) {
                auto pad = &package->pads.at(pad_uuid);
                if (pad->pool_padstack->type != Padstack::Type::MECHANICAL) {
                    auto gate_uuid = it->at("gate").get<std::string>();
                    auto pin_uuid = it->at("pin").get<std::string>();
                    if (entity->gates.count(gate_uuid)) {
                        auto gate = &entity->gates.at(gate_uuid);
                        if (gate->unit->pins.count(pin_uuid)) {
                            auto pin = &gate->unit->pins.at(pin_uuid);
                            pad_map.insert(std::make_pair(pad_uuid, PadMapItem(gate, pin)));
                        }
                    }
                }
            }
        }
    }
    version.check(ObjectType::PART, get_MPN(), uuid);
    if (j.count("tags")) {
        tags = j.at("tags").get<std::set<std::string>>();
    }
    if (j.count("parametric")) {
        const json &o = j["parametric"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            parametric.emplace(it.key(), it->get<std::string>());
        }

        if (auto pool_parametric = pool.get_parametric()) {
            if (parametric.count("table")) {
                const auto &table_name = parametric.at("table");
                const auto &tables = pool_parametric->get_tables();
                if (tables.count(table_name)) {
                    const auto &table = tables.at(table_name);
                    for (const auto &col : table.columns) {
                        if (parametric.count(col.name)) {
                            parametric_formatted.emplace(
                                    std::piecewise_construct, std::forward_as_tuple(col.name),
                                    std::forward_as_tuple(col.display_name, col.format(parametric.at(col.name))));
                        }
                    }
                }
            }
        }
    }
    if (j.count("orderable_MPNs")) {
        const json &o = j["orderable_MPNs"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            orderable_MPNs.emplace(std::string(it.key()), it->get<std::string>());
        }
    }

    init_flags(flags);

    if (j.count("flags")) {
        for (const auto &[flag_s, flag_state_s] : j.at("flags").items()) {
            const auto flag = flag_lut.lookup_opt(flag_s);
            const auto flag_state = flag_state_lut.lookup_opt(flag_state_s.get<std::string>());
            if (!flag) {
                Logger::log_warning("unknown flag " + flag_s, Logger::Domain::PART);
                continue;
            }
            if (!flag_state) {
                Logger::log_warning("unknown flag state" + flag_state_s.get<std::string>(), Logger::Domain::PART,
                                    "flag " + flag_s);
                continue;
            }
            auto flag_state2 = flag_state.value();
            if (!base && flag_state2 == FlagState::INHERIT) {
                flag_state2 = FlagState::CLEAR;
            }
            flags.at(flag.value()) = flag_state2;
        }
    }

    if (j.count("override_prefix")) {
        override_prefix = override_prefix_lut.lookup(j.at("override_prefix").get<std::string>());
        prefix = j.at("prefix").get<std::string>();
    }

    if (package->models.count(model) == 0)
        model = package->default_model;
    // if(value.size() == 0) {
    //	value = MPN;
    //}
}

const std::string &Part::get_attribute(Attribute a) const
{
    if (attributes.count(a)) {
        const auto &x = attributes.at(a);
        if (x.first && base) {
            return base->get_attribute(a);
        }
        else {
            return x.second;
        }
    }
    else {
        return empty;
    }
}

const std::string &Part::get_MPN() const
{
    return get_attribute(Attribute::MPN);
}

const std::string &Part::get_value() const
{
    const auto &r = get_attribute(Attribute::VALUE);
    if (r.size() == 0)
        return get_MPN();
    else
        return r;
}
const std::string &Part::get_manufacturer() const
{
    return get_attribute(Attribute::MANUFACTURER);
}

const std::string &Part::get_description() const
{
    return get_attribute(Attribute::DESCRIPTION);
}

const std::string &Part::get_datasheet() const
{
    return get_attribute(Attribute::DATASHEET);
}

std::set<std::string> Part::get_tags() const
{
    auto r = tags;
    if (inherit_tags && base) {
        auto tags_from_base = base->get_tags();
        r.insert(tags_from_base.begin(), tags_from_base.end());
    }
    return r;
}

UUID Part::get_model() const
{
    if (inherit_model && base)
        return base->model;
    else
        return model;
}

bool Part::get_flag(Flag fl) const
{
    const auto st = flags.at(fl);
    if (st == FlagState::INHERIT) {
        if (base) {
            return base->get_flag(fl);
        }
        else {
            return false;
        }
    }
    else {
        return st == FlagState::SET;
    }
}

const std::string &Part::get_prefix() const
{
    switch (override_prefix) {
    case OverridePrefix::INHERIT:
        if (base)
            return base->get_prefix();
        else
            return entity->prefix;

    case OverridePrefix::NO:
        return entity->prefix;

    case OverridePrefix::YES:
        return prefix;

    default:
        return entity->prefix;
    }
}

Part::OverridePrefix Part::get_override_prefix() const
{
    if (base && override_prefix == OverridePrefix::INHERIT)
        return base->get_override_prefix();
    else if (override_prefix == OverridePrefix::INHERIT)
        return OverridePrefix::NO;
    else
        return override_prefix;
}

Part::Part(const UUID &uu) : uuid(uu), version(app_version)
{
    attributes[Attribute::MPN] = {false, ""};
    attributes[Attribute::MANUFACTURER] = {false, ""};
    attributes[Attribute::VALUE] = {false, ""};
    attributes[Attribute::DATASHEET] = {false, ""};
    attributes[Attribute::DESCRIPTION] = {false, ""};
    init_flags(flags);
}

Part Part::new_from_file(const std::string &filename, IPool &pool)
{
    auto j = load_json_from_file(filename);
    return new_from_json(j, pool);
}

Part Part::new_from_json(const json &j, IPool &pool)
{
    return Part(UUID(j.at("uuid").get<std::string>()), j, pool);
}

json Part::serialize() const
{
    json j;
    const bool have_flags =
            std::count_if(flags.begin(), flags.end(), [](const auto &x) { return x.second != FlagState::CLEAR; });
    j["type"] = "part";
    j["uuid"] = (std::string)uuid;
    if (auto v = get_required_version())
        j["version"] = v;

    {
        const auto &a = attributes.at(Attribute::MPN);
        j["MPN"] = {a.first, a.second};
    }
    {
        const auto &a = attributes.at(Attribute::VALUE);
        j["value"] = {a.first, a.second};
    }
    {
        const auto &a = attributes.at(Attribute::MANUFACTURER);
        j["manufacturer"] = {a.first, a.second};
    }
    {
        const auto &a = attributes.at(Attribute::DATASHEET);
        j["datasheet"] = {a.first, a.second};
    }
    {
        const auto &a = attributes.at(Attribute::DESCRIPTION);
        j["description"] = {a.first, a.second};
    }
    j["tags"] = tags;
    j["inherit_tags"] = inherit_tags;
    j["parametric"] = parametric;
    j["model"] = (std::string)model;
    j["inherit_model"] = inherit_model;
    if (base) {
        j["base"] = (std::string)base->uuid;
    }
    else {
        j["entity"] = (std::string)entity->uuid;
        j["package"] = (std::string)package->uuid;
        j["pad_map"] = json::object();
        for (const auto &it : pad_map) {
            json k;
            k["gate"] = (std::string)it.second.gate->uuid;
            k["pin"] = (std::string)it.second.pin->uuid;
            j["pad_map"][(std::string)it.first] = k;
        }
    }
    if (orderable_MPNs.size()) {
        j["orderable_MPNs"] = json::object();
        for (const auto &it : orderable_MPNs) {
            j["orderable_MPNs"][(std::string)it.first] = it.second;
        }
    }
    if (have_flags) {
        j["flags"] = json::object();
        for (const auto [fl, st] : flags) {
            j["flags"][flag_lut.lookup_reverse(fl)] = flag_state_lut.lookup_reverse(st);
        }
    }
    if (override_prefix != OverridePrefix::NO) {
        j["override_prefix"] = override_prefix_lut.lookup_reverse(override_prefix);
        j["prefix"] = prefix;
    }
    return j;
}

UUID Part::get_uuid() const
{
    return uuid;
}

void Part::update_refs(IPool &pool)
{
    entity = pool.get_entity(entity.uuid);
    package = pool.get_package(package.uuid);
    if (base.ptr)
        base = pool.get_part(base.uuid);
    for (auto &it : pad_map) {
        it.second.gate = &entity->gates.at(it.second.gate.uuid);
        it.second.pin = &it.second.gate->unit->pins.at(it.second.pin.uuid);
    }
}

ItemSet Part::get_pool_items_used() const
{
    ItemSet items_needed;
    auto part = this;
    while (part) {
        items_needed.emplace(ObjectType::PART, part->uuid);
        items_needed.emplace(ObjectType::PACKAGE, part->package.uuid);
        for (const auto &it_pad : part->package->pads) {
            items_needed.emplace(ObjectType::PADSTACK, it_pad.second.pool_padstack->uuid);
        }
        part = part->base;
    }
    return items_needed;
}

unsigned int Part::get_required_version() const
{
    const bool have_flags =
            std::count_if(flags.begin(), flags.end(), [](const auto &x) { return x.second != FlagState::CLEAR; });
    unsigned int v = 0;
    if (have_flags)
        v = 1;
    if (override_prefix != OverridePrefix::NO)
        v = 2;
    return v;
}

} // namespace horizon
