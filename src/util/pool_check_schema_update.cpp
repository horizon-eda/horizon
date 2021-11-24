#include "pool_check_schema_update.hpp"
#include "pool/pool.hpp"
#include "widgets/forced_pool_update_dialog.hpp"
#include "util/installation_uuid.hpp"
#include "logger/logger.hpp"

namespace horizon {
CheckSchemaUpdateResult pool_check_schema_update(const std::string &base_path, Gtk::Window &parent)
{
    auto r = CheckSchemaUpdateResult::NO_UPDATE;
    try {
        Pool my_pool(base_path);
        int user_version = my_pool.db.get_user_version();
        int required_version = my_pool.get_required_schema_version();

        if (user_version != required_version)
            r = CheckSchemaUpdateResult::NEW_SCHEMA;
        else if (my_pool.get_installation_uuid() != InstallationUUID::get())
            r = CheckSchemaUpdateResult::INSTALLATION_UUID_MISMATCH;
    }
    catch (...) {
        r = CheckSchemaUpdateResult::EXCEPTION;
    }

    if (r != CheckSchemaUpdateResult::NO_UPDATE) {
        ForcedPoolUpdateDialog dia(base_path, parent);
        while (dia.run() != 1) {
        }
    }
    return r;
}
} // namespace horizon
