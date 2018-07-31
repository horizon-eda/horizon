#include "pool_notebook.hpp"
#include "editors/editor_window.hpp"
#include "duplicate/duplicate_unit.hpp"
#include "util/util.hpp"
#include "pool-prj-mgr/pool-prj-mgr-app_win.hpp"

namespace horizon {
void PoolNotebook::handle_edit_entity(const UUID &uu)
{
    if (!uu)
        return;
    UUID item_pool_uuid;
    auto path = pool.get_filename(ObjectType::ENTITY, uu, &item_pool_uuid);
    appwin->spawn(PoolProjectManagerProcess::Type::ENTITY, {path}, {}, pool_uuid && (item_pool_uuid != pool_uuid));
}

void PoolNotebook::handle_create_entity()
{
    appwin->spawn(PoolProjectManagerProcess::Type::ENTITY, {""});
}

void PoolNotebook::handle_duplicate_entity(const UUID &uu)
{
    if (!uu)
        return;
    show_duplicate_window(ObjectType::ENTITY, uu);
}
} // namespace horizon
