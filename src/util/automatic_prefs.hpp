#pragma once
#include "sqlite.hpp"

namespace horizon {
class AutomaticPreferences {
public:
    static AutomaticPreferences &get();

    const std::string db_path;
    SQLite::Database db;

private:
    AutomaticPreferences();
};
} // namespace horizon
