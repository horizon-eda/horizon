#include "rule_match_component.hpp"
#include "block/block.hpp"
#include "pool/pool.hpp"
#include "pool/part.hpp"
#include "block/net.hpp"
#include "common/lut.hpp"
#include <glibmm.h>
#include <nlohmann/json.hpp>
#include "rule.hpp"
#include "util/util.hpp"

namespace horizon {
static const LutEnumStr<RuleMatchComponent::Mode> mode_lut = {
        {"component", RuleMatchComponent::Mode::COMPONENT},
        {"components", RuleMatchComponent::Mode::COMPONENTS},
        {"part", RuleMatchComponent::Mode::PART},
};

RuleMatchComponent::RuleMatchComponent()
{
}

RuleMatchComponent::RuleMatchComponent(const json &j)
    : mode(mode_lut.lookup(j.at("mode"))), component(j.at("component").get<std::string>()),
      part(j.at("part").get<std::string>())
{
    if (j.count("components")) {
        for (const auto &it : j.at("components")) {
            components.emplace(it.get<std::string>());
        }
    }
}

json RuleMatchComponent::serialize() const
{
    json j;
    j["mode"] = mode_lut.lookup_reverse(mode);
    j["component"] = static_cast<std::string>(component);
    j["part"] = static_cast<std::string>(part);
    if (components.size()) {
        auto o = json::array();
        for (const auto &it : components) {
            o.push_back(static_cast<std::string>(it));
        }
        j["components"] = o;
    }
    return j;
}

bool RuleMatchComponent::match(const Component *c) const
{
    switch (mode) {
    case Mode::COMPONENT:
        return c && c->uuid == component;

    case Mode::COMPONENTS:
        return c && components.count(c->uuid);

    case Mode::PART:
        return c && c->part && c->part->uuid == part;
    }
    return false;
}

void RuleMatchComponent::cleanup(const Block *block)
{
    if (!block->components.count(component))
        component = UUID();
    set_erase_if(components, [block](const auto &uu) { return block->components.count(uu) == 0; });
}

bool RuleMatchComponent::matches(const class Component *c) const
{
    switch (mode) {
    case Mode::COMPONENT:
        return c->uuid == component;
    case Mode::COMPONENTS:
        return components.count(c->uuid);
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
            return "Component " + (component ? Glib::Markup::escape_text(block->components.at(component).refdes) : "?");
        }
        else {
            return "Component";
        }
    case Mode::COMPONENTS: {
        const auto sz = components.size();
        if (sz == 0)
            return "No components";
        else if (sz == 1)
            return "One component";
        else
            return std::to_string(components.size()) + " components";
    }
    case Mode::PART:
        if (pool) {
            try {
                return "Part " + (part ? Glib::Markup::escape_text(pool->get_part(part)->get_MPN()) : "?");
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
