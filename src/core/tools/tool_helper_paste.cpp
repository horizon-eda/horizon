#include "tool_helper_paste.hpp"
#include "nlohmann/json.hpp"

namespace horizon {
PackageInfo::PackageInfo(const UUID &uu, const json &j)
    : uuid(uu), placement(j.at("placement")), flip(j.at("flip").get<bool>()),
      omit_silkscreen(j.at("omit_silkscreen").get<bool>()), smashed(j.at("smashed").get<bool>()),
      group(j.at("group").get<std::string>()), tag(j.at("tag").get<std::string>()),
      package(j.at("package").get<std::string>())
{
    for (const auto &[k, v] : j.at("connections").items()) {
        connections.emplace(k, v.get<std::string>());
    }
    for (const auto &it : j.at("texts")) {
        texts.emplace(it.get<std::string>());
    }
    if (j.count("alternate_package")) {
        alternate_package = j.at("alternate_package").get<std::string>();
    }
}
} // namespace horizon
