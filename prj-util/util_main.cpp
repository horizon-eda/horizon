#include "uuid.hpp"
#include "unit.hpp"
#include "symbol.hpp"
#include "json.hpp"
#include "lut.hpp"
#include "common.hpp"
#include "pool.hpp"
#include <iostream>
#include <fstream>
#include <memory>
#include <yaml-cpp/yaml.h>
#include <stdlib.h>
#include <glibmm.h>
#include <glib/gstdio.h>
#include <glibmm/datetime.h>
#include <giomm/file.h>
#include <giomm/init.h>
#include "package.hpp"
#include "part.hpp"
#include "schematic.hpp"
#include "board.hpp"


using json = nlohmann::json;

static void save_json_to_file(const std::string &filename, const json &j) {
	std::ofstream ofs(filename);
	if(!ofs.is_open()) {
		std::cout << "can't save json " << filename <<std::endl;
		return;
	}
	ofs << std::setw(4) << j;
	ofs.close();
}

int main(int c_argc, char *c_argv[]) {
	Gio::init();

	std::vector<std::string> argv;
	for(int i = 0; i<c_argc; i++) {
		argv.emplace_back(c_argv[i]);
	}
	auto pool_base_path = Glib::getenv("HORIZON_POOL");

	if(argv.at(1) == "create-block") {
		horizon::Block block(horizon::UUID::random());
		if(argv.size() >= 4) {
			block.name = argv.at(3);
		}
		save_json_to_file(argv.at(2), block.serialize());
	}
	else if(argv.at(1) == "create-constraints") {
		horizon::Constraints constraints;
		save_json_to_file(argv.at(2), constraints.serialize());
	}
	else if(argv.at(1) == "create-schematic") {
		horizon::Pool pool(pool_base_path);
		auto constraints = horizon::Constraints::new_from_file(argv.at(4));
		auto block = horizon::Block::new_from_file(argv.at(3), pool, &constraints);
		horizon::Schematic sch(horizon::UUID::random(), block);
		save_json_to_file(argv.at(2), sch.serialize());
	}
	else if(argv.at(1) == "create-board") {
		horizon::Pool pool(pool_base_path);
		auto constraints = horizon::Constraints::new_from_file(argv.at(4));
		auto block = horizon::Block::new_from_file(argv.at(3), pool, &constraints);
		horizon::Board brd(horizon::UUID::random(), block);
		save_json_to_file(argv.at(2), brd.serialize());
	}

	return 0;
}
