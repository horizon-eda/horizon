#pragma once
#include <gtkmm.h>

namespace horizon {
enum class CheckSchemaUpdateResult { NO_UPDATE, NEW_SCHEMA, INSTALLATION_UUID_MISMATCH, EXCEPTION };
CheckSchemaUpdateResult pool_check_schema_update(const std::string &base_path, Gtk::Window &parent);
} // namespace horizon
