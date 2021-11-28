#include "rule_match_component.hpp"
#include "block/block.hpp"
#include "pool/pool.hpp"
#include "pool/part.hpp"
#include "block/net.hpp"
#include "common/lut.hpp"
#include <glibmm.h>
#include "nlohmann/json.hpp"
#include "rule.hpp"

namespace horizon {
static const LutEnumStr<RuleMatchComponent::Mode> mode_lut = {
        {"component", RuleMatchComponent::Mode::COMPONENT},
        {"part", RuleMatchComponent::Mode::PART},
};

RuleMatchComponent::RuleMatchComponent()
{
}

RuleMatchComponent::RuleMatchComponent(const json &j)
    : mode(mode_lut.lookup(j.at("mode"))), component(j.at("component").get<std::string>()),
      part(j.at("part").get<std::string>())
{
}

json RuleMatchComponent::serialize() const
{
    json j;
    j["mode"] = mode_lut.lookup_reverse(mode);
    j["component"] = static_cast<std::string>(component);
    j["part"] = static_cast<std::string>(part);

    return j;
}

bool RuleMatchComponent::match(const Component *c) const
{
    switch (mode) {
    case Mode::COMPONENT:
        return c && c->uuid == component;

    case Mode::PART:
        return c && c->part && c->part->uuid == part;
    }
    return false;
}

void RuleMatchComponent::cleanup(const Block *block)
{
    if (!block->components.count(component))
        component = UUID();
}

bool RuleMatchComponent::matches(const class Component *c) const
{
    switch (mode) {
    case Mode::COMPONENT:
        return c->uuid == component;
    case Mode::PART:
        return c->part->get_uuid() == part;
    }
    return false;
}

std::string RuleMatchComponent::get_brief(const Block *block, IPool *pool) const
{
    switch (mode) {
    case Mode::COMPONENT:
        if (block) {
            return "Component " + (component ? block->components.at(component).refdes : "?");
        }
        else {
            return "Component";
        }
    case Mode::PART:
        if (pool) {
            try {
                return "Part " + (part ? pool->get_part(part)->get_MPN() : "?");
            }
            catch (std::exception &) {
                return "Part ?";
            }
        }
        else {
            return "Part";
        }
    }
    return "";
}

bool RuleMatchComponent::can_export() const
{
    switch (mode) {
    case Mode::PART:
        return true;
    default:
        return false;
    }
}

} // namespace horizon
