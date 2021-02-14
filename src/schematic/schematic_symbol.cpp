#include "schematic_symbol.hpp"
#include "pool/part.hpp"
#include "common/lut.hpp"
#include "nlohmann/json.hpp"
#include "schematic.hpp"
#include "pool/ipool.hpp"
#include "util/util.hpp"

namespace horizon {

static const LutEnumStr<SchematicSymbol::PinDisplayMode> pdm_lut = {
        {"selected_only", SchematicSymbol::PinDisplayMode::SELECTED_ONLY},
        {"both", SchematicSymbol::PinDisplayMode::BOTH},
        {"all", SchematicSymbol::PinDisplayMode::ALL},
        {"custom_only", SchematicSymbol::PinDisplayMode::CUSTOM_ONLY},
};

SchematicSymbol::SchematicSymbol(const UUID &uu, const json &j, IPool &pool, Block *block)
    : uuid(uu), pool_symbol(pool.get_symbol(j.at("symbol").get<std::string>())), symbol(*pool_symbol),
      placement(j.at("placement")), smashed(j.value("smashed", false)),
      display_directions(j.value("display_directions", false)), display_all_pads(j.value("display_all_pads", true)),
      expand(j.value("expand", 0)), custom_value(j.value("custom_value", ""))
{
    if (j.count("pin_display_mode"))
        pin_display_mode = pdm_lut.lookup(j.at("pin_display_mode"));

    if (block) {
        component = &block->components.at(j.at("component").get<std::string>());
        gate = &component->entity->gates.at(j.at("gate").get<std::string>());
    }
    else {
        component.uuid = j.at("component").get<std::string>();
        gate.uuid = j.at("gate").get<std::string>();
    }
    if (j.count("texts")) {
        const json &o = j.at("texts");
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            texts.emplace_back(UUID(it.value().get<std::string>()));
        }
    }
}
SchematicSymbol::SchematicSymbol(const UUID &uu, const Symbol *sym)
    : uuid(uu), pool_symbol(sym), symbol(*sym), display_all_pads(false)
{
}

json SchematicSymbol::serialize() const
{
    json j;
    j["component"] = (std::string)component.uuid;
    j["gate"] = (std::string)gate.uuid;
    j["symbol"] = (std::string)pool_symbol->uuid;
    j["placement"] = placement.serialize();
    j["smashed"] = smashed;
    j["pin_display_mode"] = pdm_lut.lookup_reverse(pin_display_mode);
    j["display_directions"] = display_directions;
    j["display_all_pads"] = display_all_pads;
    j["expand"] = expand;
    j["texts"] = json::array();
    for (const auto &it : texts) {
        j["texts"].push_back((std::string)it->uuid);
    }
    if (custom_value.size())
        j["custom_value"] = custom_value;

    return j;
}

UUID SchematicSymbol::get_uuid() const
{
    return uuid;
}

std::string SchematicSymbol::get_custom_value() const
{
    return interpolate_text(custom_value, [this](const auto &var) -> std::optional<std::string> {
        std::string var_lower = var;
        std::transform(var_lower.begin(), var_lower.end(), var_lower.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        if (var_lower == "value") {
            if (component->part)
                return component->part->get_value();
            else
                return component->value;
        }
        else if (var_lower == "pkg") {
            if (component->part)
                return component->part->package->name;
            else
                return "None";
        }
        else if (var_lower == "mpn") {
            if (component->part)
                return component->part->get_MPN();
            else
                return "None";
        }
        else if (var_lower == "mfr") {
            if (component->part)
                return component->part->get_manufacturer();
            else
                return "None";
        }
        else if (var_lower == "desc") {
            if (component->part)
                return component->part->get_description();
            else
                return "None";
        }
        else if (var_lower.find("p:") == 0) {
            if (component->part) {
                const auto col = var_lower.substr(2);
                if (component->part->parametric_formatted.count(col))
                    return component->part->parametric_formatted.at(col).value;
                else
                    return {};
            }
            else {
                return "None";
            }
        }
        return {};
    });
}

std::string SchematicSymbol::replace_text(const std::string &t, bool *replaced, const Schematic &sch) const
{
    if (replaced)
        *replaced = false;
    bool is_value = t == "$VALUE";
    std::string r;
    if (t == "$REFDES" || t == "$RD") {
        if (replaced)
            *replaced = true;
        r = component->refdes + gate->suffix;
    }
    else if (is_value && custom_value.size()) {
        if (replaced)
            *replaced = true;
        r = get_custom_value();
    }
    else {
        r = component->replace_text(t, replaced);
    }
    if (is_value && sch.group_tag_visible) {
        if (component->group) {
            r += "\nG:" + sch.block->get_group_name(component->group);
            r += "\nT:" + sch.block->get_tag_name(component->tag);
        }
    }
    return r;
}

void SchematicSymbol::apply_expand()
{
    if (!pool_symbol->can_expand)
        return;
    symbol.apply_expand(*pool_symbol, expand);
}


static void append_pin_name(std::string &name, const std::string &x)
{
    if (name.size())
        name += " ";
    name += x;
}

void SchematicSymbol::apply_pin_names()
{
    for (auto &it_pin : symbol.pins) {
        it_pin.second.name.clear();
    }
    if (pin_display_mode == SchematicSymbol::PinDisplayMode::ALL) {
        for (auto &it_pin : symbol.pins) {
            auto pin_uuid = it_pin.first;
            for (auto &pin_name : gate->unit->pins.at(pin_uuid).names) {
                it_pin.second.name += pin_name + " ";
            }
            UUIDPath<2> path(gate->uuid, pin_uuid);
            if (component->custom_pin_names.count(path)) {
                it_pin.second.name += component->custom_pin_names.at(path) + " ";
            }
            it_pin.second.name += "(" + gate->unit->pins.at(pin_uuid).primary_name + ")";
        }
    }
    else if (pin_display_mode == SchematicSymbol::PinDisplayMode::CUSTOM_ONLY) {
        for (auto &it_pin : symbol.pins) {
            auto pin_uuid = it_pin.first;
            UUIDPath<2> path(gate->uuid, pin_uuid);
            if (component->custom_pin_names.count(path) && component->custom_pin_names.at(path).size()) {
                it_pin.second.name = component->custom_pin_names.at(path);
            }
            else {
                it_pin.second.name = gate->unit->pins.at(pin_uuid).primary_name;
            }
        }
    }
    else {
        for (auto &it_pin : symbol.pins) {
            auto pin_uuid = it_pin.first;
            UUIDPath<2> path(gate->uuid, pin_uuid);
            const auto &pin = gate->unit->pins.at(pin_uuid);
            if (component->pin_names.count(path) && component->pin_names.at(path).size()) {
                const auto &names = component->pin_names.at(path);
                if (names.count(-1) || (pin_display_mode == SchematicSymbol::PinDisplayMode::BOTH))
                    it_pin.second.name = gate->unit->pins.at(pin_uuid).primary_name;
                for (const auto &it : names) {
                    if (it == -2) {
                        // nop, see later
                    }
                    else if (it == -1) {
                        // nop see before
                    }
                    else {
                        if (it >= 0 && it < ((int)pin.names.size()))
                            append_pin_name(it_pin.second.name, pin.names.at(it));
                    }
                }
                if (names.count(-2) && component->custom_pin_names.count(path)) {
                    append_pin_name(it_pin.second.name, component->custom_pin_names.at(path));
                }
            }
            else {
                it_pin.second.name = gate->unit->pins.at(pin_uuid).primary_name;
            }
        }
    }
}

} // namespace horizon
