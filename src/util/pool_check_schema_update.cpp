#include "pool_check_schema_update.hpp"
#include "pool/pool.hpp"
#include "widgets/forced_pool_update_dialog.hpp"

namespace horizon {
void pool_check_schema_update(const std::string &base_path, Gtk::Window &parent)
{
    bool update_required = false;
    try {
        Pool my_pool(base_path);
        int user_version = my_pool.db.get_user_version();
        int required_version = my_pool.get_required_schema_version();
        update_required = user_version != required_version;
    }
    catch (...) {
        update_required = true;
    }
    if (update_required) {
        ForcedPoolUpdateDialog dia(base_path, parent);
        while (dia.run() != 1) {
        }
    }
}
} // namespace horizon
