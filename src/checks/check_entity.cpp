#include "check_entity.hpp"
#include "pool/entity.hpp"
#include "check_util.hpp"
#include <glibmm/regex.h>

namespace horizon {

RulesCheckResult check_entity(const Entity &entity)
{
    RulesCheckResult r;
    r.level = RulesCheckErrorLevel::PASS;

    if (!entity.name.size()) {
        r.errors.emplace_back(RulesCheckErrorLevel::FAIL, "Name must not be empty");
    }
    if (needs_trim(entity.name)) {
        r.errors.emplace_back(RulesCheckErrorLevel::FAIL, "Name has trailing/leading whitespace");
    }
    if (needs_trim(entity.manufacturer)) {
        r.errors.emplace_back(RulesCheckErrorLevel::FAIL, "Manufacturer has trailing/leading whitespace");
    }
    {
        const Glib::ustring up(entity.prefix);
        if (!Glib::Regex::match_simple("^[A-Z]+$", up)) {
            r.errors.emplace_back(RulesCheckErrorLevel::FAIL, "Prefix doesn't match regex");
        }
    }
    if (entity.tags.size() == 0) {
        r.errors.emplace_back(RulesCheckErrorLevel::FAIL, "Tags must not be empty");
    }
    for (const auto &tag : entity.tags) {
        if (!check_tag(tag)) {
            r.errors.emplace_back(RulesCheckErrorLevel::FAIL, "Tag \"" + tag + "\" doesn't match regex");
        }
    }
    static auto re_gate_suffix = Glib::Regex::create("^[A-Z]+$");
    const auto n_gates = entity.gates.size();
    if (n_gates == 0) {
        r.errors.emplace_back(RulesCheckErrorLevel::FAIL, "Entity has no gates");
    }
    else if (n_gates == 1) {
        const auto &gate = entity.gates.begin()->second;
        if (gate.name != "Main") {
            r.errors.emplace_back(RulesCheckErrorLevel::WARN, "Only gate must be named \"Main\"");
        }
        if (gate.suffix != "") {
            r.errors.emplace_back(RulesCheckErrorLevel::WARN, "Only gate must have empty suffix");
        }
        if (gate.swap_group != 0) {
            r.errors.emplace_back(RulesCheckErrorLevel::WARN, "Only gate must have zero swap group");
        }
    }
    else {
        std::set<std::string> gate_names;
        std::set<std::string> gate_suffixes;
        std::map<unsigned int, std::set<const Gate *>> gates_swap_group;
        for (const auto &[uu, gate] : entity.gates) {
            if (needs_trim(gate.name)) {
                r.errors.emplace_back(RulesCheckErrorLevel::FAIL,
                                      "Gate \"" + gate.name + "\" has trailing/leading whitespace");
            }
            if (gate_names.count(gate.name)) {
                r.errors.emplace_back(RulesCheckErrorLevel::FAIL, "Gate \"" + gate.name + "\" not unique");
            }
            gate_names.insert(gate.name);

            if (gate_suffixes.count(gate.suffix)) {
                r.errors.emplace_back(RulesCheckErrorLevel::FAIL, "Gate suffix \"" + gate.suffix + "\" not unique");
            }
            gate_suffixes.insert(gate.suffix);
            {
                Glib::ustring suffix(gate.suffix);
                if (!re_gate_suffix->match(suffix)) {
                    r.errors.emplace_back(RulesCheckErrorLevel::FAIL,
                                          "Gate suffix \"" + gate.suffix + "\" doesn't match regex");
                }
            }
            if (gate.swap_group)
                gates_swap_group[gate.swap_group].insert(&gate);
        }
        for (const auto &[swap_group, gates] : gates_swap_group) {
            if (gates.size() == 1) {
                r.errors.emplace_back(RulesCheckErrorLevel::WARN,
                                      "Swap group " + std::to_string(swap_group) + " only has one gate");
            }
            else {
                std::set<UUID> units;
                for (const auto &gate : gates) {
                    units.insert(gate->uuid);
                }
                if (units.size() > 1) {
                    r.errors.emplace_back(RulesCheckErrorLevel::WARN,
                                          "Swap group " + std::to_string(swap_group)
                                                  + " has gates with more than one distinct unit");
                }
            }
        }
    }


    r.update();
    return r;
}

} // namespace horizon
