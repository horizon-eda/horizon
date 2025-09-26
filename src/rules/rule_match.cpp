#include "rule_match.hpp"
#include "block/block.hpp"
#include "block/net.hpp"
#include "common/lut.hpp"
#include <glibmm.h>
#include <nlohmann/json.hpp>
#include "rule.hpp"
#include "util/util.hpp"

namespace horizon {
static const LutEnumStr<RuleMatch::Mode> mode_lut = {
        {"all", RuleMatch::Mode::ALL},
        {"net", RuleMatch::Mode::NET},
        {"nets", RuleMatch::Mode::NETS},
        {"net_class", RuleMatch::Mode::NET_CLASS},
        {"net_name_regex", RuleMatch::Mode::NET_NAME_REGEX},
        {"net_class_regex", RuleMatch::Mode::NET_CLASS_REGEX},
};

RuleMatch::RuleMatch()
{
}

RuleMatch::RuleMatch(const json &j)
    : mode(mode_lut.lookup(j.at("mode"))), net(j.at("net").get<std::string>()),
      net_class(j.at("net_class").get<std::string>()), net_name_regex(j.at("net_name_regex").get<std::string>()),
      net_class_regex(j.value("net_class_regex", ""))
{
    if (j.count("nets")) {
        for (const auto &it : j.at("nets")) {
            nets.emplace(it.get<std::string>());
        }
    }
}

RuleMatch::RuleMatch(const json &j, const RuleImportMap &import_map) : RuleMatch(j)
{
    net_class = import_map.get_net_class(net_class);
}

json RuleMatch::serialize() const
{
    json j;
    j["mode"] = mode_lut.lookup_reverse(mode);
    j["net"] = static_cast<std::string>(net);
    j["net_class"] = static_cast<std::string>(net_class);
    j["net_name_regex"] = net_name_regex;
    if (net_class_regex.size())
        j["net_class_regex"] = net_class_regex;
    if (nets.size()) {
        auto o = json::array();
        for (const auto &it : nets) {
            o.push_back(static_cast<std::string>(it));
        }
        j["nets"] = o;
    }
    return j;
}

bool RuleMatch::match(const Net *n) const
{
    switch (mode) {
    case Mode::ALL:
        return true;

    case Mode::NET:
        return n && n->uuid == net;

    case Mode::NETS:
        return n && nets.count(n->uuid);

    case Mode::NET_CLASS:
        return n && n->net_class->uuid == net_class;

    case Mode::NET_NAME_REGEX: {
        const Glib::ustring u(net_name_regex);
        const auto re = Glib::Regex::create(u);
        return n && re->match(n->name);
    }

    case Mode::NET_CLASS_REGEX: {
        const Glib::ustring u(net_class_regex);
        const auto re = Glib::Regex::create(u);
        return n && n->net_class && re->match(n->net_class->name);
    }
    }
    return false;
}

void RuleMatch::cleanup(const Block *block)
{
    if (!block->nets.count(net))
        net = UUID();
    if (!block->net_classes.count(net_class))
        net_class = block->net_class_default->uuid;
    set_erase_if(nets, [block](const auto &uu) { return block->nets.count(uu) == 0; });
}

std::string RuleMatch::get_brief(const Block *block) const
{
    if (block) {
        switch (mode) {
        case Mode::ALL:
            return "All";

        case Mode::NET:
            return "Net " + (net ? Glib::Markup::escape_text(block->nets.at(net).name) : "?");

        case Mode::NETS: {
            const auto sz = nets.size();
            if (sz == 0)
                return "No nets";
            else if (sz == 1)
                return "One net";
            else
                return std::to_string(nets.size()) + " nets";
        }

        case Mode::NET_CLASS:
            return "Net class " + (net_class ? Glib::Markup::escape_text(block->net_classes.at(net_class).name) : "?");

        case Mode::NET_NAME_REGEX:
            return "Net name regex";

        case Mode::NET_CLASS_REGEX:
            return "Net class regex";
        }
    }
    else {
        switch (mode) {
        case Mode::ALL:
            return "All";

        case Mode::NET:
            return "Net";

        case Mode::NETS: {
            const auto sz = nets.size();
            if (sz == 0)
                return "No nets";
            else if (sz == 1)
                return "One net";
            else
                return std::to_string(nets.size()) + " nets";
        }

        case Mode::NET_CLASS:
            return "Net class";

        case Mode::NET_NAME_REGEX:
            return "Net name regex";

        case Mode::NET_CLASS_REGEX:
            return "Net class regex";
        }
    }
    return "";
}

bool RuleMatch::can_export() const
{
    switch (mode) {
    case Mode::ALL:
    case Mode::NET_CLASS:
    case Mode::NET_NAME_REGEX:
    case Mode::NET_CLASS_REGEX:
        return true;
    default:
        return false;
    }
}

} // namespace horizon
