#include "check_part.hpp"
#include "pool/part.hpp"
#include "check_util.hpp"
#include <glibmm/regex.h>

namespace horizon {

static const std::vector<std::string> discouraged_datasheet_domains = {
        "rs-online.com", "digikey.com", "mouser.com", "farnell.com", "octopart.com",
};


static std::optional<std::string> check_datasheet(const std::string &url)
{
    for (const auto &it : discouraged_datasheet_domains) {
        if (url.find(it) != std::string::npos)
            return it;
    }
    return {};
}

RulesCheckResult check_part(const Part &part)
{
    RulesCheckResult r;
    r.level = RulesCheckErrorLevel::PASS;


    if (!part.get_MPN().size()) {
        r.errors.emplace_back(RulesCheckErrorLevel::FAIL, "MPN must not be empty");
    }
    if (needs_trim(part.get_MPN())) {
        r.errors.emplace_back(RulesCheckErrorLevel::FAIL, "MPN has trailing/leading whitespace");
    }

    if (part.get_attribute(Part::Attribute::VALUE) == part.get_MPN()) {
        r.errors.emplace_back(RulesCheckErrorLevel::WARN, "Leave value blank if it's the same as MPN");
    }
    if (needs_trim(part.get_attribute(Part::Attribute::VALUE))) {
        r.errors.emplace_back(RulesCheckErrorLevel::FAIL, "Value has trailing/leading whitespace");
    }

    if (needs_trim(part.get_manufacturer())) {
        r.errors.emplace_back(RulesCheckErrorLevel::FAIL, "Manufacturer has trailing/leading whitespace");
    }

    if (!part.get_description().size()) {
        r.errors.emplace_back(RulesCheckErrorLevel::FAIL, "Description must not be empty");
    }
    if (needs_trim(part.get_description())) {
        r.errors.emplace_back(RulesCheckErrorLevel::FAIL, "Description has trailing/leading whitespace");
    }

    if (needs_trim(part.get_datasheet())) {
        r.errors.emplace_back(RulesCheckErrorLevel::FAIL, "Description has trailing/leading whitespace");
    }
    if (auto d = check_datasheet(part.get_datasheet())) {
        r.errors.emplace_back(RulesCheckErrorLevel::WARN, "Discouraged datasheet domain " + *d);
    }

    for (const auto &[uu, mpn] : part.orderable_MPNs) {
        if (mpn == part.get_MPN()) {
            r.errors.emplace_back(RulesCheckErrorLevel::WARN, "MPN repeated in orderable MPNs");
        }
        if (needs_trim(mpn)) {
            r.errors.emplace_back(RulesCheckErrorLevel::FAIL,
                                  "Orderable MPN \"" + mpn + "\" has trailing/leading whitespace");
        }
        if (mpn.size() == 0) {
            r.errors.emplace_back(RulesCheckErrorLevel::WARN, "Orderable MPNs must not be empty");
        }
    }

    for (const auto &tag : part.tags) {
        if (!check_tag(tag)) {
            r.errors.emplace_back(RulesCheckErrorLevel::FAIL, "Tag \"" + tag + "\" doesn't match regex");
        }
    }

    {
        std::set<std::pair<UUID, UUID>> all_pins;
        for (const auto &[gate_uu, gate] : part.entity->gates) {
            for (const auto &[pin_uu, pin] : gate.unit->pins) {
                all_pins.emplace(gate_uu, pin_uu);
            }
        }
        for (const auto &[pad, pin] : part.pad_map) {
            all_pins.erase(std::make_pair(pin.gate->uuid, pin.pin->uuid));
        }
        for (const auto &[gate, pin] : all_pins) {
            r.errors.emplace_back(RulesCheckErrorLevel::WARN,
                                  "Unmapped pin " + part.entity->gates.at(gate).name + "."
                                          + part.entity->gates.at(gate).unit->pins.at(pin).primary_name);
        }
    }


    r.update();
    return r;
}

} // namespace horizon
