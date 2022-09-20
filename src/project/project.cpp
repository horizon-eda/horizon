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
#include "util/str_util.hpp"
#include "blocks/blocks_schematic.hpp"
#include "logger/logger.hpp"
#include "pool/pool_info.hpp"

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

static const unsigned int app_version = 3;

unsigned int Project::get_app_version()
{
    return app_version;
}

Project::Project(const UUID &uu, const json &j, const std::string &base)
    : base_path(base), uuid(uu), blocks_filename(Glib::build_filename(base, j.value("blocks_filename", "blocks.json"))),
      pictures_directory(Glib::build_filename(base, j.value("pictures_directory", "pictures"))),
      board_filename(Glib::build_filename(base, j.at("board_filename").get<std::string>())),
      planes_filename(Glib::build_filename(base, j.value("planes_filename", "planes.json"))),
      pool_directory(Glib::build_filename(base, j.value("pool_directory", "pool"))), version(app_version, j),
      title_old(j.value("title", "")), name_old(j.value("name", "")),
      vias_directory_old(Glib::build_filename(base, j.value("vias_directory", "vias"))),
      pool_uuid_old(j.at("pool_uuid").get<std::string>()),
      pool_cache_directory_old(Glib::build_filename(base, j.value("pool_cache_directory", "cache")))
{
    check_object_type(j, ObjectType::PROJECT);
    version.check(ObjectType::PROJECT, "", uuid);

    if (j.count("blocks")) {
        for (const auto &it : j.at("blocks")) {
            std::string block_filename = Glib::build_filename(base, it.at("block_filename").get<std::string>());
            std::string schematic_filename = Glib::build_filename(base, it.at("schematic_filename").get<std::string>());
            const auto is_top = it.at("is_top").get<bool>();

            json block_j = load_json_from_file(block_filename);

            UUID block_uuid = block_j.at("uuid").get<std::string>();
            blocks_old.emplace(block_uuid, ProjectBlock(block_uuid, block_filename, schematic_filename, is_top));
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

std::string Project::peek_title() const
{
    if (Glib::file_test(blocks_filename, Glib::FILE_TEST_IS_REGULAR)) {
        auto meta = BlocksBase::peek_project_meta(blocks_filename);
        if (meta.count("project_title"))
            return meta.at("project_title");
    }
    for (const auto &[uu, block] : blocks_old) {
        if (block.is_top && Glib::file_test(block.block_filename, Glib::FILE_TEST_IS_REGULAR)) {
            auto meta = Block::peek_project_meta(block.block_filename);
            if (meta.count("project_title"))
                return meta.at("project_title");
        }
    }

    return title_old;
}

static const std::vector<std::string> gitignore_lines = {
        "pool/*.db", "pool/*.db-*", "*.imp_meta", "*.autosave", "*.bak",
};

std::string Project::create(const std::map<std::string, std::string> &meta, const PoolInfo &pool)
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

    const auto &name = meta.at("project_name");

    BlocksSchematic blocks;
    blocks.base_path = base_path;
    auto &top_block = blocks.get_top_block_item();

    top_block.block.project_meta = meta;
    {
        auto &sheet = top_block.schematic.sheets.begin()->second;
        sheet.frame.uuid = pool.default_frame;
    }
    const auto top_block_filename = Glib::build_filename(blocks.base_path, top_block.block_filename);
    const auto top_sch_filename = Glib::build_filename(blocks.base_path, top_block.schematic_filename);
    save_json_to_file(top_block_filename, top_block.block.serialize());
    save_json_to_file(top_sch_filename, top_block.schematic.serialize());
    blocks_filename = Glib::build_filename(base_path, "blocks.json");
    save_json_to_file(blocks_filename, blocks.serialize());

    blocks_old.emplace(std::piecewise_construct, std::forward_as_tuple(top_block.uuid),
                       std::forward_as_tuple(top_block.uuid, top_block_filename, top_sch_filename, true));

    pictures_directory = Glib::build_filename(base_path, "pictures");
    pool_cache_directory_old = Glib::build_filename(base_path, "cache");
    pool_uuid_old = pool.uuid;
    pool_directory = Glib::build_filename(base_path, "pool");
    {
        mkdir_if_not_exists(pool_directory, false);
        PoolInfo info;
        info.uuid = PoolInfo::project_pool_uuid;
        info.name = "Project pool";
        info.base_path = pool_directory;
        info.pools_included = {pool.uuid};
        info.save();
        ProjectPool::create_directories(pool_directory);
    }

    Board board(UUID::random(), top_block.block);
    if (pool.default_via) {
        auto &rule_via = board.rules.add_rule_t<RuleVia>();
        rule_via.padstack = pool.default_via;
    }
    board.gerber_output_settings.prefix = name;

    board_filename = Glib::build_filename(base_path, "board.json");
    save_json_to_file(board_filename, board.serialize());

    planes_filename = Glib::build_filename(base_path, "planes.json");

    auto prj_filename = Glib::build_filename(base_path, name + ".hprj");
    save_json_to_file(prj_filename, serialize());

    {
        auto ofs = make_ofstream(Glib::build_filename(base_path, ".gitignore"));
        for (const auto &li : gitignore_lines) {
            ofs << li << "\n";
        }
    }

    return prj_filename;
}

class MyBlocks : public BlocksBase {
public:
    using BlocksBase::BlocksBase;

    class MyBlockItem : public BlockItemInfo {
    public:
        MyBlockItem(const UUID &uu, const std::string &b, const std::string &c) : BlockItemInfo(uu, b, "", c)
        {
        }
    };

    std::map<UUID, MyBlockItem> blocks;

    json serialize() const
    {
        json j = serialize_base();
        for (const auto &[uu, it] : blocks) {
            j["blocks"][(std::string)uu] = it.serialize();
        }
        return j;
    }
};

void Project::create_blocks()
{
    if (version.get_file() >= 2) {
        Logger::log_warning("create_blocks called for file version >= 2", Logger::Domain::PROJECT);
    }
    MyBlocks blocks;
    for (const auto &[uu, pb] : blocks_old) {
        blocks.blocks.emplace(std::piecewise_construct, std::forward_as_tuple(uu),
                              std::forward_as_tuple(uu, get_filename_rel(pb.block_filename),
                                                    get_filename_rel(pb.schematic_filename)));
        if (pb.is_top)
            blocks.top_block = uu;
    }
    save_json_to_file(blocks_filename, blocks.serialize());
}

static std::set<std::string> missing_lines_from_gitignore(const std::string &filename)
{
    auto ifs = make_ifstream(filename);
    if (!ifs.is_open())
        return {};

    std::set<std::string> missing_lines;
    missing_lines.insert(gitignore_lines.begin(), gitignore_lines.end());

    std::string line;
    while (std::getline(ifs, line)) {
        trim(line);
        missing_lines.erase(line);
    }
    return missing_lines;
}

bool Project::gitignore_needs_fixing(const std::string &filename)
{
    return missing_lines_from_gitignore(filename).size();
}

void Project::fix_gitignore(const std::string &filename)
{
    auto lines = missing_lines_from_gitignore(filename);
    auto ofs = make_ofstream(filename, std::ios_base::in | std::ios_base::ate);
    if (!ofs.is_open())
        return;
    for (const auto &li : lines) {
        ofs << li << "\n";
    }
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
    j["vias_directory"] = get_filename_rel(vias_directory_old);
    j["pictures_filename"] = get_filename_rel(pictures_directory);
    j["board_filename"] = get_filename_rel(board_filename);
    j["planes_filename"] = get_filename_rel(planes_filename);
    j["pool_cache_directory"] = get_filename_rel(pool_cache_directory_old);
    j["pool_directory"] = get_filename_rel(pool_directory);
    j["blocks_filename"] = get_filename_rel(blocks_filename);
    {
        json k;
        for (const auto &it : blocks_old) {
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
