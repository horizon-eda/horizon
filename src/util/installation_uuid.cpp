#include "installation_uuid.hpp"
#include "util/util.hpp"
#include <filesystem>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace horizon {
UUID InstallationUUID::get()
{
    static InstallationUUID the_installation_uuid;

    return the_installation_uuid.uuid;
}

InstallationUUID::InstallationUUID()
{
    const auto filename = fs::u8path(get_config_dir()) / "installation_uuid.json";
    if (fs::is_regular_file(filename)) {
        const auto j = load_json_from_file(filename.u8string());
        uuid = j.at("installation_uuid").get<std::string>();
    }
    else {
        uuid = UUID::random();
        const json j = {{"installation_uuid", static_cast<std::string>(uuid)}};
        save_json_to_file(filename.u8string(), j);
    }
}

} // namespace horizon
