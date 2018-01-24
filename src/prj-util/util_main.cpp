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
#include <yaml-cpp/yaml.h>

using json = nlohmann::json;

static void save_json_to_file(const std::string &filename, const json &j)
{
    std::ofstream ofs(filename);
    if (!ofs.is_open()) {
        std::cout << "can't save json " << filename << std::endl;
        return;
    }
    ofs << std::setw(4) << j;
    ofs.close();
}

int main(int c_argc, char *c_argv[])
{
    Gio::init();

    std::vector<std::string> argv;
    for (int i = 0; i < c_argc; i++) {
        argv.emplace_back(c_argv[i]);
    }
    auto pool_base_path = Glib::getenv("HORIZON_POOL");

    if (argv.at(1) == "create-block") {
        horizon::Block block(horizon::UUID::random());
        if (argv.size() >= 4) {
            block.name = argv.at(3);
        }
        save_json_to_file(argv.at(2), block.serialize());
    }
    else if (argv.at(1) == "create-schematic") {
        horizon::Pool pool(pool_base_path);
        auto block = horizon::Block::new_from_file(argv.at(3), pool);
        horizon::Schematic sch(horizon::UUID::random(), block);
        save_json_to_file(argv.at(2), sch.serialize());
    }
    else if (argv.at(1) == "create-board") {
        horizon::Pool pool(pool_base_path);
        auto block = horizon::Block::new_from_file(argv.at(3), pool);
        horizon::Board brd(horizon::UUID::random(), block);
        save_json_to_file(argv.at(2), brd.serialize());
    }

    return 0;
}
