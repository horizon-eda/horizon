#include "project.hpp"
#include <fstream>
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>
#include <giomm/file.h>
#include "util/util.hpp"
#include "block/block.hpp"
#include "schematic/schematic.hpp"
#include "board/board.hpp"
#include "nlohmann/json.hpp"
#include "pool/project_pool.hpp"

namespace horizon {

static void create_file(const std::string &filename)
{
    auto ofs = make_ofstream(filename);
    ofs.close();
}

static void mkdir_if_not_exists(const std::string &dir, bool keep)
{
    if (!Glib::file_test(dir, Glib::FILE_TEST_IS_DIR)) {
        auto fi = Gio::File::create_for_path(dir);
        fi->make_directory();
        if (keep) {
            create_file(Glib::build_filename(dir, ".keep"));
        }
    }
}

static const unsigned int app_version = 1;

unsigned int Project::get_app_version()
{
    return app_version;
}

Project::Project(const UUID &uu, const json &j, const std::string &base)
    : base_path(base), uuid(uu), vias_directory(Glib::build_filename(base, j.at("vias_directory"))),
      pictures_directory(Glib::build_filename(base, j.value("pictures_directory", "pictures"))),
      board_filename(Glib::build_filename(base, j.at("board_filename"))),
      pool_directory(Glib::build_filename(base, j.value("pool_directory", "pool"))), version(app_version, j),
      title_old(j.value("title", "")), name_old(j.value("name", "")),
      pool_uuid_old(j.at("pool_uuid").get<std::string>()),
      pool_cache_directory_old(Glib::build_filename(base, j.value("pool_cache_directory", "cache")))
{
    check_object_type(j, ObjectType::PROJECT);
    version.check(ObjectType::PROJECT, "", uuid);

    mkdir_if_not_exists(vias_directory, true);
    mkdir_if_not_exists(pictures_directory, true);

    if (j.count("blocks")) {
        const json &o = j.at("blocks");
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            const json &k = it.value();
            std::string block_filename = Glib::build_filename(base, k.at("block_filename"));
            std::string schematic_filename = Glib::build_filename(base, k.at("schematic_filename"));
            bool is_top = k.at("is_top");

            json block_j = load_json_from_file(block_filename);

            UUID block_uuid = block_j.at("uuid").get<std::string>();
            blocks.emplace(block_uuid, ProjectBlock(block_uuid, block_filename, schematic_filename, is_top));
        }
    }
}

Project Project::new_from_file(const std::string &filename)
{
    auto j = load_json_from_file(filename);
    return Project(UUID(j.at("uuid").get<std::string>()), j, Glib::path_get_dirname(filename));
}

Project::Project(const UUID &uu) : uuid(uu), version(app_version)
{
}

const ProjectBlock &Project::get_top_block() const
{
    auto top_block = std::find_if(blocks.begin(), blocks.end(), [](const auto &a) { return a.second.is_top; });
    return top_block->second;
}

ProjectBlock &Project::get_top_block()
{
    return const_cast<ProjectBlock &>(const_cast<const Project *>(this)->get_top_block());
}

std::string Project::create(const std::map<std::string, std::string> &meta, const UUID &pool_uuid,
                            const UUID &default_via)
{
    if (Glib::file_test(base_path, Glib::FILE_TEST_EXISTS)) {
        throw std::runtime_error("project directory already exists");
    }
    {
        auto fi = Gio::File::create_for_path(base_path);
        if (!fi->make_directory_with_parents()) {
            throw std::runtime_error("mkdir failed");
        }
    }

    blocks.clear();
    auto block_filename = Glib::build_filename(base_path, "top_block.json");
    auto schematic_filename = Glib::build_filename(base_path, "top_sch.json");
    auto &name = meta.at("project_name");

    Block block(UUID::random());
    block.project_meta = meta;
    save_json_to_file(block_filename, block.serialize());

    Schematic schematic(UUID::random(), block);
    save_json_to_file(schematic_filename, schematic.serialize());

    blocks.emplace(block.uuid, ProjectBlock(block.uuid, block_filename, schematic_filename, true));

    vias_directory = Glib::build_filename(base_path, "vias");
    mkdir_if_not_exists(vias_directory, true);
    pictures_directory = Glib::build_filename(base_path, "pictures");
    mkdir_if_not_exists(pictures_directory, true);
    pool_cache_directory_old = Glib::build_filename(base_path, "cache");
    pool_uuid_old = pool_uuid;
    pool_directory = Glib::build_filename(base_path, "pool");
    {
        mkdir_if_not_exists(pool_directory, false);
        PoolInfo info;
        info.uuid = PoolInfo::project_pool_uuid;
        info.name = "Project pool";
        info.base_path = pool_directory;
        info.pools_included = {pool_uuid};
        info.save();
        ProjectPool::create_directories(pool_directory);
    }

    Board board(UUID::random(), block);
    if (default_via) {
        auto rule_via = dynamic_cast<RuleVia *>(board.rules.add_rule(RuleID::VIA));
        rule_via->padstack = default_via;
    }
    board.fab_output_settings.prefix = name;

    board_filename = Glib::build_filename(base_path, "board.json");
    save_json_to_file(board_filename, board.serialize());

    auto prj_filename = Glib::build_filename(base_path, name + ".hprj");
    save_json_to_file(prj_filename, serialize());

    {
        auto ofs = make_ofstream(Glib::build_filename(base_path, ".gitignore"));
        ofs << "pool/*.db\n"
               "pool/*.db-*\n";
    }

    return prj_filename;
}

std::string Project::get_filename_rel(const std::string &p) const
{
    return Gio::File::create_for_path(base_path)->get_relative_path(Gio::File::create_for_path(p));
}

json Project::serialize() const
{
    json j;
    version.serialize(j);
    j["type"] = "project";
    j["uuid"] = (std::string)uuid;
    j["title"] = title_old;
    j["name"] = name_old;
    j["pool_uuid"] = (std::string)pool_uuid_old;
    j["vias_directory"] = get_filename_rel(vias_directory);
    j["pictures_filename"] = get_filename_rel(pictures_directory);
    j["board_filename"] = get_filename_rel(board_filename);
    j["pool_cache_directory"] = get_filename_rel(pool_cache_directory_old);
    j["pool_directory"] = get_filename_rel(pool_directory);
    {
        json k;
        for (const auto &it : blocks) {
            json l;
            l["is_top"] = it.second.is_top;
            l["block_filename"] = get_filename_rel(it.second.block_filename);
            l["schematic_filename"] = get_filename_rel(it.second.schematic_filename);
            k.push_back(l);
        }
        j["blocks"] = k;
    }

    return j;
}
} // namespace horizon
