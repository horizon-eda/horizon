#include "sqlite.hpp"


namespace SQLite {
	Database::Database(const std::string &filename, int flags) {
		if(sqlite3_open_v2(filename.c_str(), &db, flags, nullptr) != SQLITE_OK) {
			throw std::runtime_error(sqlite3_errmsg(db));
		}
	}

	void Database::execute(const std::string &query) {
		execute(query.c_str());
	}

	void Database::execute(const char *query) {
		if(sqlite3_exec(db, query, nullptr, nullptr, nullptr) != SQLITE_OK) {
			throw std::runtime_error(sqlite3_errmsg(db));
		}
	}

	Query::Query(Database &dab, const std::string &sql) : db(dab) {
		if(sqlite3_prepare_v2(db.db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
			throw std::runtime_error(sqlite3_errmsg(db.db));
		}
	}
	Query::Query(Database &dab, const char *sql, int size) : db(dab) {
		if(sqlite3_prepare_v2(db.db, sql, size, &stmt, nullptr) != SQLITE_OK) {
			throw std::runtime_error(sqlite3_errmsg(db.db));
		}
	}

	Query::~Query() {
		sqlite3_finalize(stmt);
		stmt = nullptr;
	}

	bool Query::step() {
		auto rc = sqlite3_step(stmt);
		if(rc == SQLITE_ERROR) {
			throw std::runtime_error(sqlite3_errmsg(db.db));
		}
		return rc == SQLITE_ROW;
	}

	std::string Query::get(int idx, std::string) const {
		auto c = reinterpret_cast<const char*>(sqlite3_column_text(stmt, idx));\
		if(c) {
			return c;
		}
		else {
			return "";
		}
	}

	int Query::get(int idx, int) const {
		return sqlite3_column_int(stmt, idx);
	}


	void Query::bind(int idx, const std::string &v, bool copy) {
		if(sqlite3_bind_text(stmt, idx, v.c_str(), -1, copy?SQLITE_TRANSIENT:SQLITE_STATIC) != SQLITE_OK) {
			throw std::runtime_error(sqlite3_errmsg(db.db));
		}
	}
	void Query::bind(const char *name, const std::string &v, bool copy) {
		bind(sqlite3_bind_parameter_index(stmt, name), v, copy);
	}

	void Query::bind(int idx, int v) {
		if(sqlite3_bind_int(stmt, idx, v) != SQLITE_OK) {
			throw std::runtime_error(sqlite3_errmsg(db.db));
		}
	}
	void Query::bind(const char *name, int v) {
		bind(sqlite3_bind_parameter_index(stmt, name), v);
	}

	void Query::bind(int idx, const horizon::UUID &v) {
		bind(idx, (std::string)v);
	}
	void Query::bind(const char *name, const horizon::UUID &v) {
		bind(name, (std::string)v);
	}

	Database::~Database() {
		sqlite3_close_v2(db);
	}


}
