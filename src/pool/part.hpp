#pragma once
#include "util/uuid.hpp"
#include "util/uuid_ptr.hpp"
#include <map>
#include <set>
#include "entity.hpp"
#include "package.hpp"
#include "nlohmann/json_fwd.hpp"
#include "util/file_version.hpp"
#include "util/item_set.hpp"

namespace horizon {
using json = nlohmann::json;

class Part {
private:
    Part(const UUID &uu, const json &j, class IPool &pool);
    const std::string empty;

public:
    class PadMapItem {
    public:
        PadMapItem(const class Gate *g, const class Pin *p) : gate(g), pin(p)
        {
        }
        uuid_ptr<const Gate> gate;
        uuid_ptr<const Pin> pin;
    };
    Part(const UUID &uu);

    static Part new_from_json(const json &j, IPool &pool);
    static Part new_from_file(const std::string &filename, IPool &pool);
    static unsigned int get_app_version();
    UUID uuid;

    enum class Attribute { MPN, VALUE, MANUFACTURER, DATASHEET, DESCRIPTION };
    std::map<Attribute, std::pair<bool, std::string>> attributes;
    std::map<UUID, std::string> orderable_MPNs;
    const std::string &get_attribute(Attribute a) const;
    const std::pair<bool, std::string> &get_attribute_pair(Attribute a) const;

    const std::string &get_MPN() const;
    const std::string &get_value() const;
    const std::string &get_manufacturer() const;
    const std::string &get_datasheet() const;
    const std::string &get_description() const;
    std::set<std::string> get_tags() const;
    UUID get_model() const;

    std::set<std::string> tags;
    bool inherit_tags = false;
    uuid_ptr<const class Entity> entity;
    uuid_ptr<const class Package> package;
    UUID model;
    bool inherit_model = true;
    uuid_ptr<const class Part> base;

    void update_refs(IPool &pool);
    UUID get_uuid() const;

    std::map<std::string, std::string> parametric;
    class Column {
    public:
        Column(const std::string &d, const std::string &v) : display_name(d), value(v)
        {
        }
        const std::string display_name;
        const std::string value;
    };
    std::map<std::string, Column> parametric_formatted;

    std::map<UUID, PadMapItem> pad_map;

    enum class FlagState { SET, CLEAR, INHERIT };
    enum class Flag { EXCLUDE_BOM, EXCLUDE_PNP, BASE_PART };
    std::map<Flag, FlagState> flags;
    bool get_flag(Flag fl) const;

    enum class OverridePrefix { NO, YES, INHERIT };
    OverridePrefix override_prefix = OverridePrefix::NO;
    std::string prefix;
    const std::string &get_prefix() const;
    OverridePrefix get_override_prefix() const;

    ItemSet get_pool_items_used() const;

    unsigned int get_required_version() const;

    FileVersion version;

    json serialize() const;
};
} // namespace horizon
