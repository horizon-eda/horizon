#pragma once
#include "util/uuid.hpp"
#include "nlohmann/json_fwd.hpp"
#include <map>
#include <deque>
#include "util/file_version.hpp"

namespace horizon {
using json = nlohmann::json;

class Project {
private:
    Project(const UUID &uu, const json &, const std::string &base);
    std::string get_filename_rel(const std::string &p) const;

public:
    static Project new_from_file(const std::string &filename);
    static unsigned int get_app_version();
    Project(const UUID &uu);
    std::string peek_title() const;

    std::string create(const std::map<std::string, std::string> &meta, const class PoolInfo &pool);

    static bool gitignore_needs_fixing(const std::string &filename);
    static void fix_gitignore(const std::string &filename);

    void create_blocks();

    std::string base_path;
    UUID uuid;

    std::string blocks_filename;
    std::string pictures_directory;
    std::string board_filename;
    std::string planes_filename;
    std::string pool_directory;

    FileVersion version;

    json serialize() const;

private:
    std::string title_old;
    std::string name_old;
    std::string vias_directory_old;

    UUID pool_uuid_old;
    std::string pool_cache_directory_old;

    class ProjectBlock {
    public:
        ProjectBlock(const UUID &uu, const std::string &b, const std::string &s, bool t = false)
            : uuid(uu), block_filename(b), schematic_filename(s), is_top(t)
        {
        }
        UUID uuid;
        std::string block_filename;
        std::string schematic_filename;
        bool is_top;
    };
    std::map<UUID, ProjectBlock> blocks_old;
};
} // namespace horizon
