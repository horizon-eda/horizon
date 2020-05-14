#pragma once
#include "sqlite.hpp"

namespace horizon {
class ImplicitPreferences {
public:
    static ImplicitPreferences &get();

    ImplicitPreferences();
    const std::string db_path;
    SQLite::Database db;
};
} // namespace horizon
