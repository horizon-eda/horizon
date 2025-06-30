#pragma once
#include <nlohmann/json_fwd.hpp>
#include "util/uuid.hpp"
#include <set>

namespace horizon {
using json = nlohmann::json;

class RuleMatch {
public:
    RuleMatch();
    RuleMatch(const json &j, const class RuleImportMap &import_map);
    RuleMatch(const json &j);
    json serialize() const;
    std::string get_brief(const class Block *block = nullptr) const;
    void cleanup(const class Block *block);
    bool can_export() const;

    enum class Mode { ALL, NET, NETS, NET_CLASS, NET_NAME_REGEX, NET_CLASS_REGEX };
    Mode mode = Mode::ALL;

    UUID net;
    std::set<UUID> nets;
    UUID net_class;
    std::string net_name_regex;
    std::string net_class_regex;

    bool match(const class Net *net) const;
};
} // namespace horizon
