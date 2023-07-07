#include "keepout.hpp"
#include "common/lut.hpp"
#include "nlohmann/json.hpp"
#include "object_provider.hpp"

namespace horizon {

Keepout::Keepout(const UUID &uu, const json &j, ObjectProvider &prv)
    : uuid(uu), polygon(prv.get_polygon(j.at("polygon").get<std::string>())),
      keepout_class(j.at("keepout_class").get<std::string>()), exposed_cu_only(j.at("exposed_cu_only")),
      all_cu_layers(j.at("all_cu_layers"))
{
    const json &o = j.at("patch_types_cu");
    for (auto it = o.cbegin(); it != o.cend(); ++it) {
        patch_types_cu.insert(patch_type_lut.lookup(it.value()));
    }
}

Keepout::Keepout(const UUID &uu) : uuid(uu)
{
    patch_types_cu.insert(PatchType::PAD);
    patch_types_cu.insert(PatchType::PAD_TH);
    patch_types_cu.insert(PatchType::TRACK);
    patch_types_cu.insert(PatchType::VIA);
    patch_types_cu.insert(PatchType::PLANE);
    patch_types_cu.insert(PatchType::HOLE_PTH);
}

ObjectType Keepout::get_type() const
{
    return ObjectType::KEEPOUT;
}

UUID Keepout::get_uuid() const
{
    return uuid;
}

json Keepout::serialize() const
{
    json j;
    j["polygon"] = (std::string)polygon->uuid;
    j["keepout_class"] = keepout_class;
    j["exposed_cu_only"] = exposed_cu_only;
    j["all_cu_layers"] = all_cu_layers;
    json a = json::array();
    for (const auto &it : patch_types_cu) {
        a.push_back(patch_type_lut.lookup_reverse(it));
    }
    j["patch_types_cu"] = a;
    return j;
}


} // namespace horizon
