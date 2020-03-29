#include "common/common.hpp"
#include "common/lut.hpp"
#include "pool-update/pool-update.hpp"
#include "pool/package.hpp"
#include "pool/part.hpp"
#include "pool/pool.hpp"
#include "pool/symbol.hpp"
#include "pool/unit.hpp"
#include "util/util.hpp"
#include "util/uuid.hpp"
#include "frame/frame.hpp"
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
#include "pool/pool_manager.hpp"

static void status_cb(horizon::PoolUpdateStatus st, const std::string filename, const std::string msg)
{
    switch (st) {
    case horizon::PoolUpdateStatus::DONE:
        std::cout << "done: ";
        break;
    case horizon::PoolUpdateStatus::ERROR:
        std::cout << "error: ";
        break;
    case horizon::PoolUpdateStatus::FILE:
        std::cout << "file: ";
        break;
    case horizon::PoolUpdateStatus::FILE_ERROR:
        std::cout << "file error: ";
        break;
    case horizon::PoolUpdateStatus::INFO:
        std::cout << "info: ";
        break;
    }
    std::cout << msg << " file: " << filename << std::endl;
}

int main(int c_argc, char *c_argv[])
{
    Gio::init();
    horizon::PoolManager::init();

    std::vector<std::string> argv;
    for (int i = 0; i < c_argc; i++) {
        argv.emplace_back(c_argv[i]);
    }
    auto pool_base_path = Gio::File::create_for_path(Glib::getenv("HORIZON_POOL"))->get_path();

    if (argv.size() <= 1) {
        std::cout << "Usage: " << argv.at(0) << " <command> args" << std::endl << std::endl;
        std::cout << "Available commands :" << std::endl;
        std::cout << "\tcreate-symbol <symbol file> <unit file>" << std::endl;
        std::cout << "\tcreate-package <package folder>" << std::endl;
        std::cout << "\tcreate-padstack <padstack file>" << std::endl;
        std::cout << "\tcreate-frame <frame file>" << std::endl;
        std::cout << "\tupdate" << std::endl;
        return 0;
    }

    else if (argv.at(1) == "create-symbol") {
        if (argv.size() >= 4) {
            const auto &filename_sym = argv.at(2);
            const auto &filename_unit = argv.at(3);

            horizon::Symbol sym(horizon::UUID::random());
            auto unit = horizon::Unit::new_from_file(filename_unit);
            sym.unit = &unit;
            horizon::save_json_to_file(filename_sym, sym.serialize());
        }
        else {
            std::cout << "Usage: " << argv.at(0) << " create-symbol <symbol file> <unit file>" << std::endl;
        }
    }

    else if (argv.at(1) == "create-package") {
        if (argv.size() >= 3) {
            auto &base_path = argv.at(2);
            {
                auto fi = Gio::File::create_for_path(Glib::build_filename(base_path, "padstacks"));
                fi->make_directory_with_parents();
            }
            auto pkg_filename = Glib::build_filename(base_path, "package.json");
            horizon::Package pkg(horizon::UUID::random());
            auto j = pkg.serialize();
            horizon::save_json_to_file(pkg_filename, j);
        }
        else {
            std::cout << "Usage: " << argv.at(0) << " create-package <package folder>" << std::endl;
        }
    }

    else if (argv.at(1) == "create-padstack") {
        if (argv.size() >= 3) {
            horizon::Padstack ps(horizon::UUID::random());
            auto j = ps.serialize();
            horizon::save_json_to_file(argv.at(2), j);
        }
        else {
            std::cout << "Usage: " << argv.at(0) << " create-padstack <padstack file>" << std::endl;
        }
    }

    else if (argv.at(1) == "create-frame") {
        if (argv.size() >= 3) {
            horizon::Frame fr(horizon::UUID::random());
            auto j = fr.serialize();
            horizon::save_json_to_file(argv.at(2), j);
        }
        else {
            std::cout << "Usage: " << argv.at(0) << " create-frame <frame file>" << std::endl;
        }
    }

    else if (argv.at(1) == "update") {
        horizon::pool_update(pool_base_path, status_cb);
    }

    return 0;
}
