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

YAML::Node edit_yaml(const YAML::Emitter &em)
{
    std::string name_used;
    int fd = Glib::file_open_tmp(name_used);
    write(fd, em.c_str(), em.size());
    close(fd);

    auto editor = Glib::getenv("EDITOR");
    if (editor.size() == 0) {
        editor = "vim";
    }
    std::vector<Glib::ustring> argv = {editor, name_used};
    int exit_status = -1;
    bool yaml_error = true;
    YAML::Node node;
    do {
        auto t_begin = Glib::DateTime::create_now_utc();
        Glib::spawn_sync("", argv, Glib::SPAWN_SEARCH_PATH | Glib::SPAWN_CHILD_INHERITS_STDIN, sigc::slot<void>(),
                         nullptr, nullptr, &exit_status);
        auto t_end = Glib::DateTime::create_now_utc();
        auto tdelta = t_end.to_unix() - t_begin.to_unix();
        if (tdelta < 2) {
            std::cout << "Editor exited to soon, press return to continue;" << std::endl;
            getchar();
        }
        try {
            node = YAML::LoadFile(name_used);
            yaml_error = false;
        }
        catch (const std::exception &e) {
            std::cout << "YAML error " << e.what() << std::endl;
            getchar();
            yaml_error = true;
        }
    } while (yaml_error);
    g_unlink(name_used.c_str());

    return node;
}

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

    std::vector<std::string> argv;
    for (int i = 0; i < c_argc; i++) {
        argv.emplace_back(c_argv[i]);
    }
    auto pool_base_path = Glib::getenv("HORIZON_POOL");

    if (argv.size() <= 1) {
        std::cout << "Usage: " << argv.at(0) << " <command> args" << std::endl << std::endl;
        std::cout << "Available commands :" << std::endl;
        std::cout << "\tcreate-unit <unit file>" << std::endl;
        std::cout << "\tedit-unit <unit file>" << std::endl;
        std::cout << "\tcreate-symbol <symbol file> <unit file>" << std::endl;
        std::cout << "\tcreate-entity <entity file> [unit file 1] ..." << std::endl;
        std::cout << "\tedit-entity <entity file>" << std::endl;
        std::cout << "\tcreate-package <package folder>" << std::endl;
        std::cout << "\tcreate-padstack <padstack file>" << std::endl;
        std::cout << "\tupdate" << std::endl;
        return 0;
    }

    if (argv.at(1) == "create-unit" || argv.at(1) == "edit-unit") {
        if (argv.size() >= 3) {
            const auto &filename = argv.at(2);
            horizon::Unit unit(horizon::UUID::random());
            if (argv.at(1) == "edit-unit") {
                unit = horizon::Unit::new_from_file(filename);
            }
            YAML::Emitter em;
            unit.serialize_yaml(em);
            auto node = edit_yaml(em);
            horizon::Unit unit_new(node["uuid"].as<std::string>(), node);
            auto j = unit_new.serialize();
            horizon::save_json_to_file(filename, j);
        }
        else {
            if (argv.at(1) == "create-unit") {
                std::cout << "Usage: " << argv.at(0) << " create-unit <unit file>" << std::endl;
            }
            else {
                std::cout << "Usage: " << argv.at(0) << " edit-unit <unit file>" << std::endl;
            }
        }
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

    else if (argv.at(1) == "create-entity" || argv.at(1) == "edit-entity") {
        if (argv.size() >= 3) {
            const auto &filename = argv.at(2);
            horizon::Entity entity(horizon::UUID::random());
            horizon::Pool pool(pool_base_path);
            if (argv.at(1) == "edit-entity") {
                entity = horizon::Entity::new_from_file(filename, pool);
            }
            else if (argv.size() > 3) {
                for (auto it = argv.cbegin() + 3; it < argv.cend(); it++) {
                    auto unit = horizon::Unit::new_from_file(*it);
                    auto uu = horizon::UUID::random();
                    auto &gate = entity.gates.emplace(uu, uu).first->second;
                    gate.unit = pool.get_unit(unit.uuid);
                    gate.name = "Main";
                }
            }
            YAML::Emitter em;
            entity.serialize_yaml(em);
            auto node = edit_yaml(em);
            horizon::Entity entity_new(node["uuid"].as<std::string>(), node, pool);
            auto j = entity_new.serialize();
            horizon::save_json_to_file(filename, j);
        }
        else {
            if (argv.at(1) == "create-entity") {
                std::cout << "Usage: " << argv.at(0) << " create-entity <entity file> [unit file 1] ..." << std::endl;
            }
            else {
                std::cout << "Usage: " << argv.at(0) << " edit-entity <entity file>" << std::endl;
            }
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

    else if (argv.at(1) == "update") {
        horizon::pool_update(pool_base_path, status_cb);
    }

    return 0;
}
