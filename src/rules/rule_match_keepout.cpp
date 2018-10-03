#include "rule_match_keepout.hpp"
#include "block/block.hpp"
#include "block/net.hpp"
#include "common/keepout.hpp"
#include "board/board_package.hpp"
#include "common/lut.hpp"
#include <glibmm.h>
#include "nlohmann/json.hpp"

namespace horizon {
static const LutEnumStr<RuleMatchKeepout::Mode> mode_lut = {
        {"all", RuleMatchKeepout::Mode::ALL},
        {"component", RuleMatchKeepout::Mode::COMPONENT},
        {"keepout_class", RuleMatchKeepout::Mode::KEEPOUT_CLASS},
};

RuleMatchKeepout::RuleMatchKeepout()
{
}
RuleMatchKeepout::RuleMatchKeepout(const json &j)
    : mode(mode_lut.lookup(j.at("mode"))), keepout_class(j.at("keepout_class").get<std::string>()),
      component(j.at("component").get<std::string>())
{
}

json RuleMatchKeepout::serialize() const
{
    json j;
    j["mode"] = mode_lut.lookup_reverse(mode);
    j["component"] = static_cast<std::string>(component);
    j["keepout_class"] = keepout_class;
    return j;
}


void RuleMatchKeepout::cleanup(const Block *block)
{
    if (!block->components.count(component))
        component = UUID();
}

bool RuleMatchKeepout::match(const KeepoutContour *contour) const
{
    switch (mode) {
    case Mode::ALL:
        return true;
    case Mode::KEEPOUT_CLASS:
        if (contour->pkg)
            return false;
        return keepout_class == contour->keepout->keepout_class;
    case Mode::COMPONENT:
        if (!contour->pkg)
            return false;
        return component == contour->pkg->component->uuid;
    default:
        return true;
    }
}

std::string RuleMatchKeepout::get_brief(const Block *block) const
{
    if (block) {
        switch (mode) {
        case Mode::ALL:
            return "All";

        case Mode::KEEPOUT_CLASS:
            return "Keepout class " + keepout_class;

        case Mode::COMPONENT:
            return "Component " + (component ? block->components.at(component).refdes : "?");
        }
    }
    else {
        switch (mode) {
        case Mode::ALL:
            return "All";

        case Mode::KEEPOUT_CLASS:
            return "Keepout class";

        case Mode::COMPONENT:
            return "Component";
        }
    }
    return "";
}
} // namespace horizon
