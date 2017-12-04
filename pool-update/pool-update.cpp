#include <vector>
#include <string>
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>
#include <giomm/file.h>
#include <giomm/resource.h>
#include <iostream>
#include <sstream>
#include "unit.hpp"
#include "sqlite.hpp"
#include "pool.hpp"
#include "package.hpp"
#include "part.hpp"
#include "util.hpp"
#include "pool-update.hpp"

namespace horizon {

	using json = nlohmann::json;

	static void update_units(SQLite::Database &db, const std::string &directory, pool_update_cb_t status_cb, const std::string &prefix="") {
		if(prefix.size()==0) {
			db.execute("DELETE from units");
		}
		Glib::Dir dir(directory);
		for(const auto &it: dir) {
			std::string filename = Glib::build_filename(directory, it);
			if(endswith(it, ".json")) {
				status_cb(PoolUpdateStatus::FILE, filename);
				auto unit = Unit::new_from_file(filename);
				SQLite::Query q(db, "INSERT INTO units (uuid, name, manufacturer, filename) VALUES ($uuid, $name, $manufacturer, $filename)");
				q.bind("$uuid", unit.uuid);
				q.bind("$name", unit.name);
				q.bind("$manufacturer", unit.manufacturer);
				q.bind("$filename", Glib::build_filename(prefix, it));
				q.step();
			}
			else if(Glib::file_test(filename, Glib::FILE_TEST_IS_DIR)) {
				update_units(db, filename, status_cb, Glib::build_filename(prefix, it));
			}
		}
	}

	static void update_entities(SQLite::Database &db, Pool &pool, const std::string &directory, pool_update_cb_t status_cb, const std::string &prefix="") {
		if(prefix.size()==0) {
			db.execute("DELETE from entities");
		}
		Glib::Dir dir(directory);
		for(const auto &it: dir) {
			std::string filename = Glib::build_filename(directory, it);
			if(endswith(it, ".json")) {
				status_cb(PoolUpdateStatus::FILE, filename);
				auto entity = Entity::new_from_file(filename, pool);
				SQLite::Query q(db, "INSERT INTO entities (uuid, name, manufacturer, filename, n_gates, prefix) VALUES ($uuid, $name, $manufacturer, $filename, $n_gates, $prefix)");
				q.bind("$uuid", entity.uuid);
				q.bind("$name", entity.name);
				q.bind("$manufacturer", entity.manufacturer);
				q.bind("$n_gates", entity.gates.size());
				q.bind("$prefix", entity.prefix);
				q.bind("$filename", Glib::build_filename(prefix, it));
				q.step();
				for(const auto &it_tag: entity.tags) {
					SQLite::Query q2(db, "INSERT into tags (tag, uuid, type) VALUES ($tag, $uuid, 'entity')");
					q2.bind("$uuid", entity.uuid);
					q2.bind("$tag", it_tag);
					q2.step();
				}
			}
			else if(Glib::file_test(filename, Glib::FILE_TEST_IS_DIR)) {
				update_entities(db, pool, filename, status_cb, Glib::build_filename(prefix,it));
			}
		}
	}

	static void update_symbols(SQLite::Database &db, Pool &pool, const std::string &directory, pool_update_cb_t status_cb, const std::string &prefix="") {
		if(prefix.size() == 0) {
			db.execute("DELETE from symbols");
		}
		Glib::Dir dir(directory);
		for(const auto &it: dir) {
			std::string filename = Glib::build_filename(directory, it);
			if(endswith(it, ".json")) {
				status_cb(PoolUpdateStatus::FILE, filename);
				auto symbol = Symbol::new_from_file(filename, pool);
				SQLite::Query q(db, "INSERT INTO symbols (uuid, name, filename, unit) VALUES ($uuid, $name, $filename, $unit)");
				q.bind("$uuid", symbol.uuid);
				q.bind("$name", symbol.name);
				q.bind("$unit", symbol.unit->uuid);
				q.bind("$filename", Glib::build_filename(prefix, it));
				q.step();
			}
			else if(Glib::file_test(filename, Glib::FILE_TEST_IS_DIR)) {
				update_symbols(db, pool, filename, status_cb, Glib::build_filename(prefix,it));
			}
		}
	}

	static void update_padstacks(SQLite::Database &db, const std::string &directory, pool_update_cb_t status_cb, const std::string &prefix="") {
		Glib::Dir dir(directory);
		for(const auto &it: dir) {
			auto pkgpath = Glib::build_filename(directory, it);
			auto pkgfilename = Glib::build_filename(pkgpath, "package.json");
			if(Glib::file_test(pkgfilename, Glib::FileTest::FILE_TEST_IS_REGULAR)) {
				UUID pkg_uuid;
				//we'll have to parse the package manually, since we don't have padstacks yet
				{
					std::ifstream ifs(Glib::build_filename(pkgpath, "package.json"));
					json j;
					if(!ifs.is_open()) {
						throw std::runtime_error("package not opened");
					}
					ifs>>j;
					ifs.close();
					pkg_uuid = j.at("uuid").get<std::string>();
				}

				auto padstacks_path = Glib::build_filename(pkgpath, "padstacks");
				if(Glib::file_test(padstacks_path, Glib::FileTest::FILE_TEST_IS_DIR)) {
					Glib::Dir dir2(padstacks_path);
					for(const auto &it2: dir2) {
						if(endswith(it2, ".json")) {
							std::string filename = Glib::build_filename(padstacks_path, it2);
							status_cb(PoolUpdateStatus::FILE, filename);
							auto padstack = Padstack::new_from_file(filename);
							SQLite::Query q(db, "INSERT INTO padstacks (uuid, name, filename, package, type) VALUES ($uuid, $name, $filename, $package, $type)");
							q.bind("$uuid", padstack.uuid);
							q.bind("$name", padstack.name);
							q.bind("$type", Padstack::type_lut.lookup_reverse(padstack.type));
							q.bind("$package", pkg_uuid);
							q.bind("$filename", Glib::build_filename("packages", prefix, it, "padstacks", it2));
							q.step();
						}
					}
				}
			}
			else if(Glib::file_test(pkgpath, Glib::FILE_TEST_IS_DIR)) {
				update_padstacks(db, pkgpath, status_cb, Glib::build_filename(prefix,it));
			}

		}
	}

	static void update_padstacks_global(SQLite::Database &db, const std::string &directory, pool_update_cb_t status_cb, const std::string &prefix="") {
		if(prefix.size()==0) {
			db.execute("DELETE from padstacks");
		}
		Glib::Dir dir(directory);
		for(const auto &it: dir) {
			std::string filename = Glib::build_filename(directory, it);
			if(endswith(it, ".json")) {
				status_cb(PoolUpdateStatus::FILE, filename);
				auto padstack = Padstack::new_from_file(filename);
				SQLite::Query q(db, "INSERT INTO padstacks (uuid, name, filename, package, type) VALUES ($uuid, $name, $filename, $package, $type)");
				q.bind("$uuid", padstack.uuid);
				q.bind("$name", padstack.name);
				q.bind("$type", Padstack::type_lut.lookup_reverse(padstack.type));
				q.bind("$package", UUID());
				q.bind("$filename", Glib::build_filename("padstacks", prefix, it));
				q.step();
			}
			else if(Glib::file_test(filename, Glib::FILE_TEST_IS_DIR)) {
				update_padstacks(db, filename, status_cb, Glib::build_filename(prefix,it));
			}



		}
	}

	static void update_packages(SQLite::Database &db, Pool &pool, const std::string &directory, pool_update_cb_t status_cb, const std::string &prefix="") {
		if(prefix.size()==0) {
			db.execute("DELETE from packages");
		}
		Glib::Dir dir(directory);
		for(const auto &it: dir) {
			auto pkgpath = Glib::build_filename(directory, it);
			auto pkgfilename = Glib::build_filename(pkgpath, "package.json");
			if(Glib::file_test(pkgfilename, Glib::FileTest::FILE_TEST_IS_REGULAR)) {
				std::string filename = Glib::build_filename(pkgpath, "package.json");
				status_cb(PoolUpdateStatus::FILE, filename);
				auto package = Package::new_from_file(filename, pool);
				SQLite::Query q(db, "INSERT INTO packages (uuid, name, manufacturer, filename, n_pads, alternate_for) VALUES ($uuid, $name, $manufacturer, $filename, $n_pads, $alt_for)");
				q.bind("$uuid", package.uuid);
				q.bind("$name", package.name);
				q.bind("$manufacturer", package.manufacturer);
				q.bind("$n_pads", std::count_if(package.pads.begin(), package.pads.end(), [](const auto &x){return x.second.padstack.type != Padstack::Type::MECHANICAL;}));
				q.bind("$alt_for", package.alternate_for?package.alternate_for->uuid:UUID());
				q.bind("$filename", Glib::build_filename(prefix, it, "package.json"));
				q.step();
				for(const auto &it_tag: package.tags) {
					SQLite::Query q2(db, "INSERT into tags (tag, uuid, type) VALUES ($tag, $uuid, 'package')");
					q2.bind("$uuid", package.uuid);
					q2.bind("$tag", it_tag);
					q2.step();
				}
			}
			else if(Glib::file_test(pkgpath, Glib::FILE_TEST_IS_DIR)) {
				update_packages(db, pool, pkgpath, status_cb, Glib::build_filename(prefix,it));
			}
		}
	}

	static bool update_parts(SQLite::Database &db, Pool &pool, const std::string &directory, pool_update_cb_t status_cb, const std::string &prefix="") {
		if(prefix.size()==0)
			db.execute("DELETE from parts");

		Glib::Dir dir(directory);
		bool skipped = true;
		while(skipped) {
			skipped = false;
			if(prefix.size() == 0)
				db.execute("BEGIN TRANSACTION");
			for(const auto &it: dir) {
				std::string filename = Glib::build_filename(directory, it);
				if(endswith(it, ".json")) {
					bool skipthis = false;
					{
						status_cb(PoolUpdateStatus::FILE, filename);
						std::ifstream ifs(filename);
						json j;
						if(!ifs.is_open()) {
							throw std::runtime_error("part not opened");
						}
						ifs>>j;
						ifs.close();
						if(j.count("base")) {
							UUID base_uuid = j.at("base").get<std::string>();
							SQLite::Query q(pool.db, "SELECT uuid from parts WHERE uuid = $uuid");
							q.bind("$uuid", base_uuid);
							if(!q.step()) { //not found
								skipthis = true;
								skipped = true;
							}
						}
					}
					if(!skipthis) {
						status_cb(PoolUpdateStatus::FILE, filename);
						auto part = Part::new_from_file(filename, pool);
						SQLite::Query q(db, "INSERT INTO parts (uuid, MPN, manufacturer, entity, package, filename) VALUES ($uuid, $MPN, $manufacturer, $entity, $package, $filename)");
						q.bind("$uuid", part.uuid);
						q.bind("$MPN", part.get_MPN());
						q.bind("$manufacturer", part.get_manufacturer());
						q.bind("$package", part.package->uuid);
						q.bind("$entity", part.entity->uuid);
						q.bind("$filename", Glib::build_filename(prefix, it));
						q.step();

						for(const auto &it_tag: part.get_tags()) {
							SQLite::Query q2(db, "INSERT into tags (tag, uuid, type) VALUES ($tag, $uuid, 'part')");
							q2.bind("$uuid", part.uuid);
							q2.bind("$tag", it_tag);
							q2.step();
						}
					}
				}
				else if(Glib::file_test(filename, Glib::FILE_TEST_IS_DIR)) {
					if(update_parts(db, pool, filename, status_cb, Glib::build_filename(prefix,it)))
						skipped = true;
				}
			}
			if(prefix.size() == 0)
				db.execute("COMMIT");
			if(prefix.size()>0)
				break;
		}
		return skipped;

	}

	void status_cb_nop(PoolUpdateStatus st, const std::string msg) {}

	static const int min_user_version = 1; //keep in sync with schema

	void pool_update(const std::string &pool_base_path, pool_update_cb_t status_cb) {
		auto pool_db_path = Glib::build_filename(pool_base_path, "pool.db");
		if(!status_cb)
			status_cb = &status_cb_nop;

		status_cb(PoolUpdateStatus::INFO, "start");

		SQLite::Database db(pool_db_path, SQLITE_OPEN_CREATE|SQLITE_OPEN_READWRITE);
		{
			int user_version = 0;
			{
				SQLite::Query q(db, "PRAGMA user_version");
				if(q.step()) {
					user_version = q.get<int>(0);
				}
			}
			if(user_version < min_user_version) {
				//update schema
				auto bytes = Gio::Resource::lookup_data_global("/net/carrotIndustries/horizon/pool-update/schema.sql");
				gsize size {bytes->get_size()+1};//null byte
				auto data = (const char*)bytes->get_data(size);
				db.execute(data);
				status_cb(PoolUpdateStatus::INFO, "created db from schema");
			}
		}

		Pool pool(pool_base_path);

		status_cb(PoolUpdateStatus::INFO, "tags");
		db.execute("BEGIN TRANSACTION");
		db.execute("DELETE FROM tags");
		update_units(db, Glib::build_filename(pool_base_path, "units"), status_cb);
		db.execute("COMMIT");

		status_cb(PoolUpdateStatus::INFO, "entities");
		db.execute("BEGIN TRANSACTION");
		update_entities(db, pool, Glib::build_filename(pool_base_path, "entities"), status_cb);
		db.execute("COMMIT");

		status_cb(PoolUpdateStatus::INFO, "symbols");
		db.execute("BEGIN TRANSACTION");
		update_symbols(db, pool, Glib::build_filename(pool_base_path, "symbols"), status_cb);
		db.execute("COMMIT");

		status_cb(PoolUpdateStatus::INFO, "padstacks");
		db.execute("BEGIN TRANSACTION");
		update_padstacks_global(db, Glib::build_filename(pool_base_path, "padstacks"), status_cb);
		update_padstacks(db, Glib::build_filename(pool_base_path, "packages"), status_cb);
		db.execute("COMMIT");

		status_cb(PoolUpdateStatus::INFO, "packages");
		db.execute("BEGIN TRANSACTION");
		update_packages(db, pool, Glib::build_filename(pool_base_path, "packages"), status_cb);
		db.execute("COMMIT");

		status_cb(PoolUpdateStatus::INFO, "parts");
		update_parts(db, pool, Glib::build_filename(pool_base_path, "parts"), status_cb);
		status_cb(PoolUpdateStatus::DONE, "done");
	}
}
