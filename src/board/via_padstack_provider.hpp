#pragma once
#include "common/common.hpp"
#include "nlohmann/json_fwd.hpp"
#include "pool/padstack.hpp"
#include "util/uuid.hpp"
#include <fstream>
#include <map>
#include <sqlite3.h>
#include <vector>

namespace horizon {

class ViaPadstackProvider {
public:
    ViaPadstackProvider(const std::string &p, class IPool &po);
    const Padstack *get_padstack(const UUID &uu);
    void update_available();
    class PadstackEntry {
    public:
        PadstackEntry(const std::string &p, const std::string &n) : path(p), name(n)
        {
        }

        std::string path;
        std::string name;
    };
    const std::map<UUID, PadstackEntry> &get_padstacks_available() const;

private:
    std::string base_path;
    class IPool &pool;

    std::map<UUID, Padstack> padstacks;
    std::map<UUID, PadstackEntry> padstacks_available;
};
} // namespace horizon
