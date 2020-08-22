#include "via_padstack_provider.hpp"
#include "pool/ipool.hpp"
#include "util/util.hpp"
#include "util/sqlite.hpp"
#include <glibmm.h>
#include <glibmm/fileutils.h>
#include "nlohmann/json.hpp"

namespace horizon {
ViaPadstackProvider::ViaPadstackProvider(const std::string &bp, IPool &po) : base_path(bp), pool(po)
{
    update_available();
}

void ViaPadstackProvider::update_available()
{
    Glib::Dir dir(base_path);
    padstacks_available.clear();
    for (const auto &it : dir) {
        std::string filename = Glib::build_filename(base_path, it);
        if (endswith(filename, ".json")) {
            auto j = load_json_from_file(filename);
            padstacks_available.emplace(UUID(j.at("uuid").get<std::string>()),
                                        PadstackEntry(filename, j.at("name").get<std::string>() + " (local)"));
        }
    }
    SQLite::Query q(pool.get_db(),
                    "SELECT padstacks.uuid, padstacks.name FROM padstacks "
                    "WHERE padstacks.type = 'via'");
    while (q.step()) {
        padstacks_available.emplace(UUID(q.get<std::string>(0)), PadstackEntry("", q.get<std::string>(1)));
    }
}

const std::map<UUID, ViaPadstackProvider::PadstackEntry> &ViaPadstackProvider::get_padstacks_available() const
{
    return padstacks_available;
}

const Padstack *ViaPadstackProvider::get_padstack(const UUID &uu)
{
    if (padstacks.count(uu)) {
        return &padstacks.at(uu);
    }
    if (padstacks_available.count(uu)) {
        const auto &it = padstacks_available.at(uu);
        if (it.path.size()) {
            padstacks.emplace(uu, Padstack::new_from_file(it.path));
            return &padstacks.at(uu);
        }
        else {
            return pool.get_padstack(uu);
        }
    }
    return nullptr;
}
} // namespace horizon
