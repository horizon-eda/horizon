#include "board/board.hpp"
#include "common/common.hpp"
#include "common/lut.hpp"
#include "pool/package.hpp"
#include "pool/part.hpp"
#include "pool/pool.hpp"
#include "pool/symbol.hpp"
#include "pool/unit.hpp"
#include "schematic/schematic.hpp"
#include "util/uuid.hpp"
#include <fstream>
#include <giomm/file.h>
#include <giomm/init.h>
#include <glib/gstdio.h>
#include <glibmm.h>
#include <glibmm/datetime.h>
#include <iostream>
#include <memory>
#include <stdlib.h>
#include "nlohmann/json.hpp"
#include "util/util.hpp"

using json = nlohmann::json;

int main(int c_argc, char *c_argv[])
{
    Gio::init();

    std::vector<std::string> argv;
    for (int i = 0; i < c_argc; i++) {
        argv.emplace_back(c_argv[i]);
    }
    auto pool_base_path = Glib::getenv("HORIZON_POOL");

    if (argv.size() <= 1) {
        std::cout << "Usage: " << argv.at(0) << " <command> args" << std::endl << std::endl;
        std::cout << "Available commands :" << std::endl;
        std::cout << "\tcreate-block <block file> [block name]" << std::endl;
        std::cout << "\tcreate-schematic <block file> <schematic file>" << std::endl;
        std::cout << "\tcreate-board <block file> <board file>" << std::endl;
        return 0;
    }

    if (argv.at(1) == "create-block") {
        if (argv.size() >= 3) {
            horizon::Block block(horizon::UUID::random());
            if (argv.size() >= 4) {
                block.name = argv.at(3);
            }
            horizon::save_json_to_file(argv.at(2), block.serialize());
        }
        else {
            std::cout << "Usage: " << argv.at(0) << " create-block <block file> [block name]" << std::endl;
        }
    }
    else if (argv.at(1) == "create-schematic") {
        if (argv.size() >= 4) {
            horizon::Pool pool(pool_base_path);
            auto block = horizon::Block::new_from_file(argv.at(3), pool);
            horizon::Schematic sch(horizon::UUID::random(), block);
            horizon::save_json_to_file(argv.at(2), sch.serialize());
        }
        else {
            std::cout << "Usage: " << argv.at(0) << " create-schematic <block file> <schematic file>" << std::endl;
        }
    }
    else if (argv.at(1) == "create-board") {
        if (argv.size() >= 4) {
            horizon::Pool pool(pool_base_path);
            auto block = horizon::Block::new_from_file(argv.at(3), pool);
            horizon::Board brd(horizon::UUID::random(), block);
            horizon::save_json_to_file(argv.at(2), brd.serialize());
        }
        else {
            std::cout << "Usage: " << argv.at(0) << " create-board <block file> <board file>" << std::endl;
        }
    }

    return 0;
}
