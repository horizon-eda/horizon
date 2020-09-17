#include "check_unit.hpp"
#include "pool/unit.hpp"
#include "check_util.hpp"
#include <glibmm/regex.h>

namespace horizon {

RulesCheckResult check_unit(const Unit &unit)
{
    RulesCheckResult r;
    r.level = RulesCheckErrorLevel::PASS;

    if (!unit.name.size()) {
        r.errors.emplace_back(RulesCheckErrorLevel::FAIL, "Name must not be empty");
    }
    if (needs_trim(unit.name)) {
        r.errors.emplace_back(RulesCheckErrorLevel::FAIL, "Name has trailing/leading whitespace");
    }
    if (needs_trim(unit.manufacturer)) {
        r.errors.emplace_back(RulesCheckErrorLevel::FAIL, "Manufacturer has trailing/leading whitespace");
    }
    std::set<std::string> pin_names;
    for (const auto &[uu, pin] : unit.pins) {
        if (pin_names.count(pin.primary_name)) {
            r.errors.emplace_back(RulesCheckErrorLevel::FAIL, "Pin \"" + pin.primary_name + "\" not unique");
        }
        pin_names.insert(pin.primary_name);
        if (needs_trim(pin.primary_name)) {
            r.errors.emplace_back(RulesCheckErrorLevel::FAIL,
                                  "Pin \"" + pin.primary_name + "\" has trailing/leading whitespace");
        }
        std::set<std::string> names;
        for (const auto &name : pin.names) {
            if (names.count(name)) {
                r.errors.emplace_back(RulesCheckErrorLevel::FAIL,
                                      "Alt. name \"" + name + "\" of pin \"" + pin.primary_name + "\" not unique");
            }
            names.insert(name);
        }
        if (names.count(pin.primary_name)) {
            r.errors.emplace_back(RulesCheckErrorLevel::FAIL,
                                  "Alt. name of pin \"" + pin.primary_name + "\" must not repeat primary name");
        }
    }


    r.update();
    return r;
}

} // namespace horizon
