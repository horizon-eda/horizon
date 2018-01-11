#include "project.hpp"
#include <fstream>
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>
#include <giomm/file.h>
#include "util/util.hpp"
#include "block/block.hpp"
#include "schematic/schematic.hpp"
#include "board/board.hpp"

namespace horizon {
	Project::Project(const UUID &uu, const json &j, const std::string &base):
			base_path(base),
			uuid(uu),
			name(j.at("name").get<std::string>()),
			title(j.at("title").get<std::string>()),
			pool_uuid(j.at("pool_uuid").get<std::string>()),
			vias_directory(Glib::build_filename(base, j.at("vias_directory"))),
			board_filename(Glib::build_filename(base, j.at("board_filename"))),
			pool_cache_directory(Glib::build_filename(base, j.value("pool_cache_directory", "cache")))
		{
			if(!Glib::file_test(pool_cache_directory, Glib::FILE_TEST_IS_DIR)) {
				auto fi = Gio::File::create_for_path(pool_cache_directory);
				fi->make_directory();
			}
			if(j.count("blocks")){
				const json &o = j.at("blocks");
				for (auto it = o.cbegin(); it != o.cend(); ++it) {
					const json &k = it.value();
					std::string block_filename = Glib::build_filename(base, k.at("block_filename"));
					std::string schematic_filename = Glib::build_filename(base, k.at("schematic_filename"));
					bool is_top = k.at("is_top");

					json block_j;
					std::ifstream ifs(block_filename);
					if(!ifs.is_open()) {
						throw std::runtime_error("file "  +block_filename+ " not opened");
					}
					ifs>>block_j;
					ifs.close();

					UUID block_uuid = block_j.at("uuid").get<std::string>();
					blocks.emplace(block_uuid, ProjectBlock(block_uuid, block_filename, schematic_filename, is_top));


				}
			}
		}

	Project Project::new_from_file(const std::string &filename) {
		json j;
		std::ifstream ifs(filename);
		if(!ifs.is_open()) {
			throw std::runtime_error("file "  +filename+ " not opened");
		}
		ifs>>j;
		ifs.close();
		return Project(UUID(j["uuid"].get<std::string>()), j, Glib::path_get_dirname(filename));
	}

	Project::Project(const UUID &uu): uuid(uu) {
	}

	ProjectBlock &Project::get_top_block() {
		auto top_block = std::find_if(blocks.begin(), blocks.end(), [](const auto &a){return a.second.is_top;});
		return top_block->second;
	}

	std::string Project::create(const UUID &default_via) {
		if(Glib::file_test(base_path, Glib::FILE_TEST_EXISTS)) {
			throw std::runtime_error("project directory already exists");
		}
		{
			auto fi = Gio::File::create_for_path(base_path);
			if(!fi->make_directory_with_parents()) {
				throw std::runtime_error("mkdir failed");
			}
		}

		blocks.clear();
		auto block_filename = Glib::build_filename(base_path, "top_block.json");
		auto schematic_filename = Glib::build_filename(base_path, "top_sch.json");

		Block block(UUID::random());
		save_json_to_file(block_filename, block.serialize());

		Schematic schematic(UUID::random(), block);
		save_json_to_file(schematic_filename, schematic.serialize());

		blocks.emplace(block.uuid, ProjectBlock(block.uuid, block_filename, schematic_filename, true));

		vias_directory = Glib::build_filename(base_path, "vias");
		{
			auto fi = Gio::File::create_for_path(vias_directory);
			fi->make_directory();
		}
		pool_cache_directory= Glib::build_filename(base_path, "cache");
		{
			auto fi = Gio::File::create_for_path(pool_cache_directory);
			fi->make_directory();
		}

		Board board(UUID::random(), block);
		if(default_via) {
			auto rule_via = dynamic_cast<RuleVia*>(board.rules.add_rule(RuleID::VIA));
			rule_via->padstack = default_via;
		}
		board.fab_output_settings.prefix = name;

		board_filename = Glib::build_filename(base_path, "board.json");
		save_json_to_file(board_filename, board.serialize());

		auto prj_filename = Glib::build_filename(base_path, name+".hprj");
		save_json_to_file(prj_filename, serialize());
		return prj_filename;
	}

	std::string Project::get_filename_rel(const std::string &p) const {
		return Gio::File::create_for_path(base_path)->get_relative_path(Gio::File::create_for_path(p));
	}

	json Project::serialize() const {
		json j;
		j["type"] = "project";
		j["uuid"] = (std::string) uuid;
		j["name"] = name;
		j["title"] = title;
		j["pool_uuid"] = (std::string)pool_uuid;
		j["vias_directory"] = get_filename_rel(vias_directory);
		j["board_filename"] = get_filename_rel(board_filename);
		j["pool_cache_directory"] = get_filename_rel(pool_cache_directory);
		{
			json k;
			for(const auto &it: blocks) {
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
}
