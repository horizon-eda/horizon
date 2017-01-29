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
#include "json.hpp"

using json = nlohmann::json;

bool endswidth(const std::string &haystack, const std::string &needle) {
	return (haystack.size()-haystack.rfind(needle)) == needle.size();
}


void update_resistor(SQLite::Database &db, const horizon::Part *part) {
	SQLite::Query q(db, "INSERT INTO resistors (part, value, pmax, tolerance) VALUES ($part, $value, $pmax, $tolerance)");
	q.bind("$part", part->uuid);
	q.bind("$value", part->parametric.at("value"));
	q.bind("$pmax", part->parametric.at("pmax"));
	q.bind("$tolerance", part->parametric.at("tolerance"));
	q.step();
}
void update_capacitor(SQLite::Database &db, const horizon::Part *part) {
	SQLite::Query q(db, "INSERT INTO capacitors (part, value, wvdc, characteristic) VALUES ($part, $value, $wvdc, $characteristic)");
	q.bind("$part", part->uuid);
	q.bind("$value", part->parametric.at("value"));
	q.bind("$wvdc", part->parametric.at("wvdc"));
	q.bind("$characteristic", part->parametric.at("characteristic"));
	q.step();
}

int main(int c_argc, char *c_argv[]) {
	std::vector<std::string> argv;
	for(int i = 0; i<c_argc; i++) {
		argv.emplace_back(c_argv[i]);
	}
	auto pool_base_path = Glib::getenv("HORIZON_POOL");
	auto pool_db_path = Glib::build_filename(pool_base_path, "pool.db");
	auto pool_para_path = Glib::build_filename(pool_base_path, "parametric.db");
	SQLite::Database db(pool_db_path);
	SQLite::Database db_parametric(pool_para_path, SQLITE_OPEN_CREATE|SQLITE_OPEN_READWRITE);
	{
		SQLite::Query q(db_parametric, "SELECT name FROM sqlite_master WHERE type='table' AND name='resistors'");
		if(!q.step()) { //db is likely empty
			auto bytes = Gio::Resource::lookup_data_global("/net/carrotIndustries/horizon/pool-update-parametric/schema.sql");
			gsize size {bytes->get_size()+1};//null byte
			auto data = (const char*)bytes->get_data(size);
			db_parametric.execute(data);
			std::cout << "created db from schema" << std::endl;
		}
	}
	
	
	horizon::Pool pool(pool_base_path);

	db_parametric.execute("BEGIN TRANSACTION");
	db_parametric.execute("DELETE FROM resistors");
	db_parametric.execute("DELETE FROM capacitors");

	SQLite::Query q(db, "SELECT uuid from parts");
	while(q.step()) {
		auto part = pool.get_part(q.get<std::string>(0));
		if(part->parametric.count("table")) {
			std::string table = part->parametric.at("table");
			if(table == "resistors") {
				update_resistor(db_parametric, part);
			}
			if(table == "capacitors") {
				update_capacitor(db_parametric, part);
			}
		}
	}
	db_parametric.execute("COMMIT");




	return 0;
}
