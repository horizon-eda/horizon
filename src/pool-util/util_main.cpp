#include "util/uuid.hpp"
#include "pool/unit.hpp"
#include "pool/symbol.hpp"
#include "common/lut.hpp"
#include "common/common.hpp"
#include "pool/pool.hpp"
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
#include "pool/package.hpp"
#include "pool/part.hpp"
#include "util/util.hpp"
#include "pool-update/pool-update.hpp"



YAML::Node edit_yaml(const YAML::Emitter &em) {
	std::string name_used;
	int fd = Glib::file_open_tmp(name_used);
	write(fd, em.c_str(), em.size());
	close(fd);

	auto editor = Glib::getenv("EDITOR");
	if(editor.size() == 0) {
		editor = "vim";
	}
	std::vector<Glib::ustring> argv = {editor, name_used};
	int exit_status = -1;
	bool yaml_error = true;
	YAML::Node node;
	do {
		auto t_begin = Glib::DateTime::create_now_utc();
		Glib::spawn_sync("", argv, Glib::SPAWN_SEARCH_PATH|Glib::SPAWN_CHILD_INHERITS_STDIN , sigc::slot<void>(), nullptr, nullptr, &exit_status);
		auto t_end = Glib::DateTime::create_now_utc();
		auto tdelta = t_end.to_unix() -t_begin.to_unix();
		if(tdelta<2) {
			std::cout << "Editor exited to soon, press return to continue;" << std::endl;
			getchar();
		}
		try {
			node = YAML::LoadFile(name_used);
			yaml_error = false;
		}
		catch(const std::exception& e) {
			std::cout << "YAML error " << e.what() << std::endl;
			getchar();
			yaml_error = true;
		}
	} while(yaml_error);
	g_unlink(name_used.c_str());


	return node;
}


static void status_cb(horizon::PoolUpdateStatus st, const std::string msg) {
	switch(st) {
		case horizon::PoolUpdateStatus::DONE :
			std::cout << "done: ";
		break;
		case horizon::PoolUpdateStatus::ERROR :
			std::cout << "error: ";
		break;
		case horizon::PoolUpdateStatus::FILE :
			std::cout << "file: ";
		break;
		case horizon::PoolUpdateStatus::INFO :
			std::cout << "info: ";
		break;
	}
	std::cout << msg << std::endl;
}

int main(int c_argc, char *c_argv[]) {
	Gio::init();

	std::vector<std::string> argv;
	for(int i = 0; i<c_argc; i++) {
		argv.emplace_back(c_argv[i]);
	}
	auto pool_base_path = Glib::getenv("HORIZON_POOL");


	if(argv.at(1) == "create-unit" || argv.at(1) == "edit-unit") {
		const auto &filename = argv.at(2);
		horizon::Unit unit(horizon::UUID::random());
		if(argv.at(1) == "edit-unit") {
			unit = horizon::Unit::new_from_file(filename);
		}
		YAML::Emitter em;
		unit.serialize_yaml(em);
		auto node = edit_yaml(em);
		horizon::Unit unit_new (node["uuid"].as<std::string>(), node);
		auto j = unit_new.serialize();
		horizon::save_json_to_file(filename, j);
	}
	else if(argv.at(1) == "create-symbol") {
		const auto &filename_sym = argv.at(2);
		const auto &filename_unit = argv.at(3);

		horizon::Symbol sym(horizon::UUID::random());
		auto unit = horizon::Unit::new_from_file(filename_unit);
		sym.unit = &unit;
		horizon::save_json_to_file(filename_sym, sym.serialize());
	}

	else if(argv.at(1) == "create-entity" || argv.at(1) == "edit-entity") {
		const auto &filename = argv.at(2);
		horizon::Entity entity(horizon::UUID::random());
		horizon::Pool pool(pool_base_path);
		if(argv.at(1) == "edit-entity") {
			entity = horizon::Entity::new_from_file(filename, pool);
		}
		else {
			if(argv.size()>3) {
				for(auto it = argv.cbegin()+3; it<argv.cend(); it++) {
					auto unit = horizon::Unit::new_from_file(*it);
					auto uu = horizon::UUID::random();
					auto &gate = entity.gates.emplace(uu, uu).first->second;
					gate.unit = pool.get_unit(unit.uuid);
					gate.name = "Main";
				}
			}
		}
		YAML::Emitter em;
		entity.serialize_yaml(em);
		auto node = edit_yaml(em);
		horizon::Entity entity_new (node["uuid"].as<std::string>(), node, pool);
		auto j = entity_new.serialize();
		horizon::save_json_to_file(filename, j);
	}

	else if(argv.at(1) == "create-package") {
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

	else if(argv.at(1) == "create-padstack") {
		horizon::Padstack ps(horizon::UUID::random());
		auto j = ps.serialize();
		horizon::save_json_to_file(argv.at(2), j);
	}

	else if(argv.at(1) == "update") {
		horizon::pool_update(pool_base_path, status_cb);
	}



	return 0;
}
